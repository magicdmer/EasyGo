#include "FastSearch.h"
#include <map>
#include <stack>
#include <Windows.h>
#include "ChineseLetterHelper.h"
#include "LogFile.h"

using namespace tyrlib;

#define VOLUMN_ROOT 0x5000000000005

FastSearch::FastSearch()
{
}

FastSearch::~FastSearch()
{

}

inline HANDLE FastSearch::CreateVolHandle(const WCHAR szVol)
{
	WCHAR szVolFormat[MAX_PATH] = {0};
	swprintf(szVolFormat, L"\\\\.\\%c:", szVol);
    return CreateFileW(
		szVolFormat,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
}

inline void FastSearch::AddToMap(const USN_RECORD *pRec, const WCHAR *szVol)
{
    TYR_USN_RECORD rec;
    memset(&rec,0,sizeof(TYR_USN_RECORD));
    rec.frn = pRec->FileReferenceNumber;
    rec.pFrn = pRec->ParentFileReferenceNumber;
    rec.dwAttribute = pRec->FileAttributes;
    rec.cDrive = szVol[0];
    memcpy(rec.szFileName, pRec->FileName, pRec->FileNameLength);
    _pResults[pRec->FileReferenceNumber] = rec;
}

bool FastSearch::Traverse(const WCHAR *szVol, USN *nextUSN)
{
	HANDLE hVol = CreateVolHandle(szVol[0]);
    if (hVol == INVALID_HANDLE_VALUE)
    {
        qLog(QString("Open drive %1: failed, error:%2").arg(szVol[0]).arg(GetLastError()));
        return false;
    }

    MFT_ENUM_DATA_V0 mft_enum_data = {0};
    USN_JOURNAL_DATA journal_data = {0};
	USN_RECORD *pRecord = NULL;
	DWORD dwRetBytes = 0;

	if(!DeviceIoControl(hVol,
		FSCTL_QUERY_USN_JOURNAL,
		NULL,
		0,
		&journal_data,
		sizeof(journal_data),
		&dwRetBytes,
        NULL))
    {
        qLog(QString("DeviceIoControl FSCTL_QUERY_USN_JOURNAL failed. %1").arg(GetLastError()));
        CloseHandle(hVol);
        return false;
	}

	*nextUSN = mft_enum_data.HighUsn = journal_data.NextUsn;

    const int nBufferSize = 0x1000;
    char buffer[0x1000] = {0};

    for(;;)
    {
        memset(buffer, 0, nBufferSize);

		if(!DeviceIoControl(hVol,
			FSCTL_ENUM_USN_DATA,
			&mft_enum_data,
			sizeof(mft_enum_data),
			buffer,
			nBufferSize,
			&dwRetBytes,
            NULL))
        {
            if (GetLastError() != ERROR_HANDLE_EOF)
            {
                qLog(QString("DeviceIoControl FSCTL_ENUM_USN_DATA failed. %1").arg(GetLastError()));
                CloseHandle(hVol);
                return false;
            }

            break;
		}

        dwRetBytes = dwRetBytes - sizeof(USN);
        pRecord = (PUSN_RECORD)((PCHAR)buffer + sizeof(USN));
        while(dwRetBytes > 0)
        {
            if(IsValuableFile(pRecord))
            {
				AddToMap(pRecord, szVol);
			}
			dwRetBytes -= pRecord->RecordLength;
            pRecord = (PUSN_RECORD)((PCHAR)pRecord + pRecord->RecordLength);
		}
        mft_enum_data.StartFileReferenceNumber = *(USN*)&buffer;
	}

    CloseHandle(hVol);
    return true;
}

void FastSearch::SaveToDataBase(IndexDatabase* pDatabase)
{
	WCHAR szFullPath[MAX_PATH] = {0};
	size_t nPath = MAX_PATH;

    int count = 0;
    pDatabase->beginInsert();
    for(RecMI mi = _pResults.begin(); mi != _pResults.end(); mi++)
    {
        if (mi->second.dwAttribute & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }

		memset(szFullPath,0,MAX_PATH);
        int ret = GetFullPath(mi->second.frn, mi->second.cDrive, szFullPath, &nPath);
        if (!ret)
		{
            nPath = MAX_PATH;
            if (!GetFullPathByFrn(mi->second.frn, mi->second.cDrive, szFullPath,&nPath))
            {
                continue;
            }
		}

        if (wcsnicmp(szFullPath,L"C:\\Windows\\winsxs",17) == 0 ||
            wcsnicmp(szFullPath,L"C:\\Windows\\servicing",20) == 0 ||
            wcsnicmp(szFullPath,L"C:\\$Recycle.Bin",15) == 0)
		{
			continue;
		}

        ProgramInfo info;
        info.name = QString::fromWCharArray(mi->second.szFileName);
        info.pyname = ChineseLetterHelper::GetFirstLetters(info.name);
        info.path = QString::fromWCharArray(szFullPath);

        if (count % 5000 == 0 && count / 5000 > 0)
        {
            pDatabase->endInsert();
            pDatabase->beginInsert();
        }

        pDatabase->insert(info);

        count++;
	}
    pDatabase->endInsert();

    _pResults.clear();
}

bool FastSearch::IsValuableFile(const USN_RECORD* pRecord)
{
    if (pRecord->FileNameLength/2 >= 128)
    {
        return false;
    }

    if (pRecord->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return true;
    }
    else
    {
        if (pRecord->FileNameLength < 10 || pRecord->FileNameLength > 120)
        {
            return false;
        }
    }

	WCHAR fileName[128] = {0};
	memset(fileName,0,128);
	memcpy(fileName, pRecord->FileName, pRecord->FileNameLength);
	
	int pos = (pRecord->FileNameLength)/2 - 4;
    if (0 != wcsnicmp(&fileName[pos],L".exe",4) &&
        0 != wcsnicmp(&fileName[pos],L".lnk",4) &&
        0 != wcsnicmp(&fileName[pos],L".bat",4))
	{
		return false;
	}

	return true;
}

bool FastSearch::GetFullPathByFrn(DWORDLONG frn,WCHAR vol,WCHAR *szFullPath,size_t *nFullPathLen)
{
    if (_pResults.count(frn) == 0)
    {
        return false;
    }

    std::stack<TYR_USN_RECORD> stackName;

    bool findRoot = false;
    DWORDLONG ulFrn = frn;
    while(_pResults.count(ulFrn))
    {
        TYR_USN_RECORD& rec = (_pResults)[ulFrn];
        if (rec.pFrn == VOLUMN_ROOT)
        {
            findRoot = true;
        }
        stackName.push(rec);
        ulFrn = rec.pFrn;
    }

    if (!findRoot)
    {
        return false;
    }

    WCHAR *fullPath = new WCHAR[1024];
    if (fullPath == NULL)
    {
        return false;
    }

    swprintf(fullPath,L"%c:",vol);

    while(!stackName.empty()) {
        TYR_USN_RECORD rec = stackName.top();
        wcscat(fullPath,L"\\");
        wcscat(fullPath,rec.szFileName);
        stackName.pop();
    }

    int fullLen = wcslen(fullPath);
    if (fullLen >= MAX_PATH)
    {
        delete[] fullPath;
        *nFullPathLen = fullLen + 1;
        return false;
    }

    wcscpy(szFullPath,fullPath);

    delete[] fullPath;

    return true;
}

bool FastSearch::GetFullPath(IN DWORDLONG frn,
							IN WCHAR vol,
							OUT WCHAR *szFullPath,
							IN OUT size_t *nFullPathLen)
{
	HANDLE hVol = CreateVolHandle(vol);
	if (hVol == NULL)
	{
        return false;
	}

    FILE_ID_DESCRIPTOR fileDescriptor;
    fileDescriptor.Type = FileIdType;
    fileDescriptor.FileId.QuadPart = frn;
    fileDescriptor.dwSize = sizeof(fileDescriptor);
    HANDLE hFile = OpenFileById(hVol,&fileDescriptor, 0, 0, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hVol);
        return false;
    }

	PFILE_NAME_INFO info = (PFILE_NAME_INFO)new char[sizeof(FILE_NAME_INFO)+128];
	BOOL ret = GetFileInformationByHandleEx(hFile,FileNameInfo,info,sizeof(FILE_NAME_INFO)+128);
	if (!ret)
	{
		delete info;
		CloseHandle(hFile);
		CloseHandle(hVol);
        return false;
	}

	if (info->FileNameLength > *nFullPathLen - 6)
	{
        delete info;
        CloseHandle(hFile);
        CloseHandle(hVol);
		*nFullPathLen = info->FileNameLength;
        return false;
	}

	swprintf(szFullPath,L"%c:",vol);
	memcpy(szFullPath + 2,info->FileName,info->FileNameLength);

	delete info;
	CloseHandle(hFile);
	CloseHandle(hVol);

    return true;
}
