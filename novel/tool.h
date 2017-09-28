#pragma once

#include <string>
#include <vector>
#include <regex>
#include "curl/curl.h"
#pragma comment(lib, "libcurl_a.lib") 

typedef struct tagDownResourceInfo
{
	int				rowNum;					//行号
	std::wstring	strResourceName;		//资源名称
	std::wstring	strResourceLink;		//资源链接
}DownResourceInfo;

static void StringToWstring(std::wstring& szDst, std::string& str)
{
	std::string temp = str;
	int len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)temp.c_str(), -1, NULL, 0);
	wchar_t * wszUtf8 = new wchar_t[len + 1];
	memset(wszUtf8, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)temp.c_str(), -1, (LPWSTR)wszUtf8, len);
	szDst = wszUtf8;
	delete[] wszUtf8;
}

static BOOL UrlEncode(const wchar_t* szSrc, char* pBuf, int cbBufLen, BOOL bUpperCase)
{
	if (szSrc == NULL || pBuf == NULL || cbBufLen <= 0)
		return FALSE;

	size_t len_unicode = wcslen(szSrc);
	if (len_unicode == 0)
	{
		pBuf[0] = 0;
		return TRUE;
	}

	char baseChar = bUpperCase ? 'A' : 'a';

	int cbUTF8 = WideCharToMultiByte(CP_UTF8, 0, szSrc, len_unicode, NULL, 0, NULL, NULL);
	LPSTR pUTF8 = (LPSTR)malloc((cbUTF8 + 1) * sizeof(CHAR));
	if (pUTF8 == NULL)
	{
		return FALSE;
	}
	WideCharToMultiByte(CP_UTF8, 0, szSrc, len_unicode, pUTF8, cbUTF8 + 1, NULL, NULL);
	pUTF8[cbUTF8] = '\0';

	unsigned char c;
	int cbDest = 0; //累加
	unsigned char *pSrc = (unsigned char*)pUTF8;
	unsigned char *pDest = (unsigned char*)pBuf;
	while (*pSrc && cbDest < cbBufLen - 1)
	{
		c = *pSrc;
		if (isalpha(c) || isdigit(c) || c == '-' || c == '.' || c == '~')
		{
			*pDest = c;
			++pDest;
			++cbDest;
		}
		else if (c == ' ')
		{
			*pDest = '+';
			++pDest;
			++cbDest;
		}
		else
		{
			//检查缓冲区大小是否够用？
			if (cbDest + 3 > cbBufLen - 1)
				break;
			pDest[0] = '%';
			pDest[1] = (c >= 0xA0) ? ((c >> 4) - 10 + baseChar) : ((c >> 4) + '0');
			pDest[2] = ((c & 0xF) >= 0xA) ? ((c & 0xF) - 10 + baseChar) : ((c & 0xF) + '0');
			pDest += 3;
			cbDest += 3;
		}
		++pSrc;
	}
	//null-terminator
	*pDest = '\0';
	free(pUTF8);
	return TRUE;
}

static long writer(void *data, int size, int nmemb, std::string &content)
{
	long sizes = size * nmemb;
	std::string temp((char*)data, sizes);
	content += temp;
	return sizes;
}

/************************************************************************
* 函数作用：获取搜索小说
* 参数说明：关键字
* 返 回 值：EnumResult
* 备注信息：无
************************************************************************/
static bool InitCurl(CURL *easy_handle, CURLcode &res, std::string &url, std::string &content)
{

	res = curl_easy_setopt(easy_handle, CURLOPT_URL, url.c_str());
	if (res != CURLE_OK)
		return FALSE;

	//回调函数
	res = curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, writer);
	if (res != CURLE_OK)
		return FALSE;

	//回调函数的参数：content
	res = curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &content);
	if (res != CURLE_OK)
		return FALSE;

	// set ssl
	curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);

	//执行http请求
	res = curl_easy_perform(easy_handle);
	if (res != CURLE_OK)
		return FALSE;

	return TRUE;
}

static std::wstring GetSearchPage(std::string url)
{
	CURL *easy_handle;
	CURLcode res;
	std::string content;
	curl_global_init(CURL_GLOBAL_ALL);
	easy_handle = curl_easy_init();
	if (easy_handle)
	{
		//curl_easy_setopt(easy_handle, CURLOPT_FOLLOWLOCATION, 1L);

		if (!InitCurl(easy_handle, res, url, content))
		{
			//释放资源
			curl_easy_cleanup(easy_handle);
			return NULL;
		}
		//释放资源
		curl_easy_cleanup(easy_handle);
	}
	curl_global_cleanup();
	// 将内容有UTF-8转换为WString
	std::wstring tt;
	StringToWstring(tt, content);
	return tt;
}

//获取小说章节
//<a.*([0 - 9]{ 2, }.html).*>([\u4e00 - \u9fa5 \（\）0 - 9\.]{ 1, })< / a>

//获取小说内容

//获取搜索小说列表
static std::vector<DownResourceInfo> GetNovelList(wchar_t* search_name)
{
	std::vector<DownResourceInfo> vtDownload;
	char encodesearch[256];
	BOOL isSucess = UrlEncode(search_name, encodesearch, 256, true);
	if (!isSucess)
	{
		return vtDownload;
	}
	char url[256];
	sprintf_s(url, "www.baidu.com/s?wd=%s", encodesearch);
	std::wstring html = GetSearchPage(url);
	int nPos = 0;
	int n = 0;
	int flag = 1;
	while ((nPos = html.find(L"se_st_com_abstract", n)) != -1)
	{
		n = nPos + 1;
		int nStartPos = nPos;
		int nEndPos = html.find(L"</div></div>", nStartPos);
		std::wstring strNovel = html.substr(nStartPos, nEndPos - nStartPos);

		// 保存小说数据
		DownResourceInfo info;
		
		// 正则表达解析小说数据
		const std::wregex pattern(L"\"title\":\"(.*)\",\"url\":\"(.*)\"");
		std::wsmatch result;

		for (std::wsregex_iterator it(strNovel.begin(), strNovel.end(), pattern), end;     //end是尾后迭代器，regex_iterator是regex_iterator的string类型的版本
			it != end;
			++it)
		{
			// 获取正则表达式中 小说名称
			info.strResourceName = (*it)[1].str();
			// 获取正则表达式中 小说链接
			info.strResourceLink = (*it)[2].str();
		}
		// 获取正则表达式中 小说序号
		info.rowNum = flag;
		vtDownload.push_back(info);
		++flag;
	}
	return vtDownload;
}
