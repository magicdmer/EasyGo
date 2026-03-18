# -*- coding: utf-8 -*-
from __future__ import print_function
import json
import sys
import inspect

class PythonPlugin(object):
    """
    EasyGo python plugin base
    """

    def __init__(self):
        rpc_request = json.loads(sys.argv[1])
        # proxy is not working now
        # self.proxy = rpc_request.get("proxy",{})
        request_method_name = rpc_request.get("FuncName")
        request_parameter = rpc_request.get("Parameter")
        methods = inspect.getmembers(self, predicate=inspect.ismethod)

        request_method = dict(methods)[request_method_name]
        results = request_method(request_parameter)

        if request_method_name == "Query" or request_method_name == "GetContextMenu":
            print(json.dumps({"Results": results}))
        elif request_method_name == "InitPlugin":
            print(json.dumps({"Result":results}))
            
    def InitPlugin(self,pluginPath):
        return True
        
    def Query(self,query):
        """
        sub class need to override this method
        """
        return []

    def GetContextMenu(self, data):
        """
        optional context menu entries for a result
        """
        return []

class PythonPluginAPI(object):

    @classmethod
    def ChangeQuery(cls,query):
        """
        change EasyGo query
        """
        print(json.dumps({"FuncName": "Ra_ChangeQuery","Parameter":query}))
    
    @classmethod
    def Reload(cls):
        """
        reload EasyGo
        """
        print(json.dumps({"FuncName": "Ra_Reload","Parameter":""}))
    
    @classmethod
    def ReQuery(cls):
        """
        let EasyGo requery
        """
        print(json.dumps({"FuncName": "Ra_ReQuery","Parameter":""}))
        
    @classmethod
    def ShowMsg(cls,title,msg):
        """
        let EasyGo show MsgBox
        """
        param = json.dumps({"Title":title,"Msg":msg})
        print(json.dumps({"FuncName": "Ra_ShowMsg","Parameter":param}))
    
    @classmethod
    def ShowTip(cls,title,msg):
        """
        let EasyGo show TipBox
        """
        param = json.dumps({"Title":title,"Msg":msg})
        print(json.dumps({"FuncName": "Ra_ShowTip","Parameter":param}))
    
    @classmethod
    def ShowContent(cls,title,msg):
        """
        let EasyGo show readonly content dialog
        """
        param = json.dumps({"Title":title,"Msg":msg})
        print(json.dumps({"FuncName": "Ra_ShowContent","Parameter":param}))
        
    @classmethod
    def EditFile(cls,title,filePath):
        """
        let EasyGo show text file edit dialog
        """
        param = json.dumps({"Title":title,"FilePath":filePath})
        print(json.dumps({"FuncName": "Ra_EditFile","Parameter":param}))