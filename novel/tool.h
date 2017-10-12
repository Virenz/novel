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

typedef struct tagNovelChapter
{
	std::wstring	strChapterName;		//章节名称
	std::wstring	strChapterLink;		//章节链接
}NovelChapter;

static void StringToWstring(std::wstring& szDst, std::string& str, UINT codepage)
{
	std::string temp = str;
	DWORD len = MultiByteToWideChar(codepage, 0, temp.c_str(), -1, NULL, 0);
	wchar_t * wszUtf8 = new wchar_t[len + 1];
	memset(wszUtf8, 0, (len+1)*sizeof(wchar_t));
	MultiByteToWideChar(codepage, 0, temp.c_str(), -1, wszUtf8, len);
	szDst = wszUtf8;
	delete[] wszUtf8;
}

static void Wchar_tToString(std::string& szDst, wchar_t *wchar)
{
	wchar_t * wText = wchar;
	DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);// WideCharToMultiByte的运用
	char *psText;  // psText为char*的临时数组，作为赋值给std::string的中间变量
	psText = new char[dwNum];
	WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);// WideCharToMultiByte的再次运用
	szDst = psText;// std::string赋值
	delete[]psText;// psText的清除
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

// 小说网址拼凑链接，主要针对首页+章节的组合获取完整的网址
static std::wstring getFullChapterLink(std::wstring& realurl, std::wstring& chapterLink)
{
	if (chapterLink.find(L"http") != -1)
	{
		return chapterLink;
	}

	// 切割当前网页真实网址以"/"结尾
	int realurl_pos = realurl.rfind(L"/");
	std::wstring m_realurl = realurl.substr(0, realurl_pos+1);

	int chapter_pos = chapterLink.rfind(L"/");
	std::wstring m_chapter;
	if(chapter_pos != -1)
		m_chapter = chapterLink.substr(chapter_pos+1, wcslen(chapterLink.c_str())- chapter_pos - 1);
	else
		m_chapter = chapterLink.substr(0, wcslen(chapterLink.c_str()));

	return m_realurl+m_chapter;
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
	{
		return FALSE;
	}
	return TRUE;
}

// 传入网址url，传出跳转真实网址realurl
// 返回网页源代码
static std::wstring GetSearchPage(std::string url)
{
	//struct curl_slist *head = NULL;

	CURL *easy_handle;
	CURLcode res;
	std::string content;
	std::wstring tt;
	std::string real_url;
	curl_global_init(CURL_GLOBAL_ALL);
	easy_handle = curl_easy_init();

	if (easy_handle)
	{
		curl_easy_setopt(easy_handle, CURLOPT_FOLLOWLOCATION, 1L);
		//head = curl_slist_append(head, "Content-Type:application/x-www-form-urlencoded;charset=UTF-8");
		//curl_easy_setopt(easy_handle, CURLOPT_HTTPHEADER, head);

		if (!InitCurl(easy_handle, res, url, content))
		{
			//curl_slist_free_all(head);//记得要释放 
			//释放资源
			curl_easy_cleanup(easy_handle);
			return tt;
		}
		char *realurl = NULL;
		curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &realurl);
		real_url.append(realurl);
		//curl_slist_free_all(head);//记得要释放 
		//释放资源
		curl_easy_cleanup(easy_handle);
	}
	curl_global_cleanup();
	// 将内容转换为WString, 判断网页源码的charset
	UINT codepage = 0;
	if(-1 != content.find("charset=gbk",0))
	{
		codepage = 0;
	}
	if(-1 != content.find("charset=GBK", 0))   
	{
		codepage = 0;
	}
	if(-1 != content.find("charset=gb2312", 0))
	{
		codepage = 0;
	}
	if(-1 != content.find("charset=GB2312", 0))
	{
		codepage = 0;
	}
	if(-1 != content.find("charset=utf-8", 0))
	{
		codepage = 65001;
	}
	if(-1 != content.find("charset=UTF-8", 0))
	{
		codepage = 65001;
	}
	
	content.append("REAL_URL:[" + real_url + "]");
	StringToWstring(tt, content, codepage);

	return tt;
}

//获取小说章节
static std::vector<NovelChapter> GetNoveChapters(wchar_t* chapter_link)
{
	std::vector<NovelChapter> vtNovelChapters;
	
	std::string url;
	Wchar_tToString(url, chapter_link);
	std::wstring html = GetSearchPage(url);

	// 保存小说章节
	NovelChapter info;

	// 获取真实链接
	int nStartPos = html.rfind(L"REAL_URL:\[", html.size());
	int nEndPos = html.rfind(L"\]", html.size());
	std::wstring strrealurl = html.substr(nStartPos+10, nEndPos - nStartPos-10);

	// 正则表达解析小说数据
	// (L"<a[^/]*\"([0-9a-z/_]+.html)\"[^/]*>([\u4e00-\u9fa5 \\《\\》\\（\\）0-9\.\\，\\！\\【\\】\\？\\~]{1,})</a>");
	// (L"<a.*\"([0-9a-z/_]+.html)\".*>([\u4e00-\u9fa5 \\《\\》\\（\\）0-9\.\\，]{1,})</a>");
	const std::wregex pattern(L"<a[^/]*\"([0-9a-z/_\:\.]+.html)\"[^/]*>([\u4e00-\u9fa5 \|\\《\\》\\（\\）?0-9〇⓪①②③④⑤⑥⑦⑧⑨⑩\.\\，\\。\\！\\【\\】\\？\\“\\”\\…\\、\\：\\~\\→\\☆]{1,})</a>"); 
	std::wsmatch result;

	for (std::wsregex_iterator it(html.begin(), html.end(), pattern), end;     //end是尾后迭代器，regex_iterator是regex_iterator的string类型的版本
		it != end;
		++it)
	{
		// 获取正则表达式中 小说每个章节名称
		info.strChapterName = (*it)[2].str();
		// 获取正则表达式中 小说每个章节链接
		info.strChapterLink = getFullChapterLink(strrealurl,(*it)[1].str());
		vtNovelChapters.push_back(info);
	}
	return vtNovelChapters;
}

//获取小说章节内容
//&nbsp;&nbsp;&nbsp;&nbsp;([\u4e00-\u9fa5\S]{1,})<br
static std::wstring GetChapterContent(wchar_t* chapter_link)
{
	std::string url;
	Wchar_tToString(url, chapter_link);
	std::wstring html = GetSearchPage(url);

	// 正则表达解析小说章节内容
	const std::wregex pattern(L"(&nbsp;&nbsp;&nbsp;&nbsp;|&rdquo;|)*([\u4e00-\u9fa5 \\《\\》\\（\\）0-9\.\\，\\。\\！\\？\\“\\”\\…\\、\\：\\~\*]{1,})(<br|</p>)");
	std::wsmatch result;
	std::wstring contents;

	for (std::wsregex_iterator it(html.begin(), html.end(), pattern), end;     //end是尾后迭代器，regex_iterator是regex_iterator的string类型的版本
		it != end;
		++it)
	{
		// 获取正则表达式中 小说每个章节名称
		contents.append((*it)[2].str());
		contents.append(L"\r\n");
	}
	return contents;
}

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
	while ((nPos = html.find(L"\"result c-container \"", n)) != -1)
	{
		n = nPos + 10;
		int nStartPos = nPos;
		int nEndPos = html.find(L"</div></div>", nStartPos);
		std::wstring strNovel = html.substr(nStartPos, nEndPos - nStartPos);
		// 保存小说数据
		DownResourceInfo info;
		
		// 正则表达解析小说数据
		const std::wregex pattern(L"\\{\"title\":\"([^/]*)\",\"url\":\"([^\u4e00-\u9fa5]*)\"\\}");
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
