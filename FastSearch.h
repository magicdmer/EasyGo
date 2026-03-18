
#ifndef _TYR_FASTSEARCH_H_
#define _TYR_FASTSEARCH_H_

#undef _UNICODE
#undef UNICODE

#include <stdio.h>
#include <Windows.h>
#include <map>
#include "IndexDatabase.h"

namespace tyrlib
{
	typedef struct _st_TYR_USN_RECORD {
		DWORDLONG frn;
		DWORDLONG pFrn;
		DWORD dwAttribute;
		WCHAR cDrive;
		WCHAR szFileName[128];
	} TYR_USN_RECORD;

    typedef std::map<DWORDLONG, TYR_USN_RECORD>::iterator RecMI;
    typedef std::map<DWORDLONG, TYR_USN_RECORD> RecMap, *PRecMap;

	class FastSearch
	{
	public:
		FastSearch();
		virtual ~FastSearch();
		
        bool Traverse(const WCHAR *szVol, USN *nextUSN);
        void SaveToDataBase(IndexDatabase* database);

	private:
        std::map<DWORDLONG,TYR_USN_RECORD> _pResults;

	private:
		HANDLE CreateVolHandle(const WCHAR szVol);
        void AddToMap(const USN_RECORD *pRec, const WCHAR *szVol);
        bool GetFullPath(IN DWORDLONG frn, IN WCHAR vol, OUT WCHAR *szFullPath, IN OUT size_t *nFullPathLen);
        bool GetFullPathByFrn(IN DWORDLONG frn,IN WCHAR vol,OUT WCHAR *szFullPath, IN OUT size_t *nFullPathLen);
		bool IsValuableFile(const USN_RECORD* record);

	private:
		FastSearch(const FastSearch&);
		FastSearch &operator=(const FastSearch&);
	};
};

#endif
