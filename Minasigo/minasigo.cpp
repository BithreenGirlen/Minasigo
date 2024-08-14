

#include <Windows.h>
#include <urlmon.h>
#include <atlbase.h>

#include <string>
#include <vector>

#include "minasigo.h"
#include "json_minimal.h"
#include "text_utility.h"
#include "win_http_session.h"
#include "main_window.h"

#include "deps/unix_clock.h"
#include "deps/md5.h"
#include "deps/base64.h"
#include "deps/sha256.h"
#include "deps/hmac.h"

#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "shlwapi.lib")

namespace minasigo
{
	struct ResourcePath
	{
		std::string strMd5;
		std::string strFilePath;
	};

	struct MnsgVersion
	{
		std::string strResourceVersion;
		std::string strMasterVersion;
		std::string strClientVersion;
	};

	struct Authorisation
	{
		std::string strConsumerKey;
		std::string strConsumerSecret;
		std::string strUserId;
		std::string strToken;
		std::string strSecret;
	};

	MnsgVersion g_MnsgVersion;
	Authorisation g_Auth;

	CMainWindow* g_pMainWindow = nullptr;

	/*======================== Copy but converted from wchar_t* to char* ========================*/

	/*伝文出力*/
	void WriteMessage(const char* msg)
	{
		char stamp[16];
		SYSTEMTIME tm;
		::GetLocalTime(&tm);
		sprintf_s(stamp, "%02d:%02d:%02d:%03d ", tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
		std::string str = stamp + std::string(msg) + "\r\n";
		printf(str.c_str());
	}
	/*実行プロセスの階層取得*/
	std::string GetBaseFolderPath()
	{
		char pzPath[MAX_PATH]{};
		::GetModuleFileNameA(nullptr, pzPath, MAX_PATH);
		std::string::size_type pos = std::string(pzPath).find_last_of("\\/");
		return std::string(pzPath).substr(0, pos) + "\\/";
	}
	/*作業フォルダ作成*/
	std::string CreateWorkFolder(const char* pzFolderName)
	{
		std::string strFolder = GetBaseFolderPath();
		if (pzFolderName != nullptr)
		{
			strFolder += pzFolderName;
			strFolder += "\\/";
			::CreateDirectoryA(strFolder.c_str(), nullptr);
		}
		return strFolder;
	}
	/*ファイルのメモリ展開*/
	char* LoadExistingFile(const char* pzFilPath)
	{
		HANDLE hFile = ::CreateFileA(pzFilPath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwSize = ::GetFileSize(hFile, nullptr);
			if (dwSize != INVALID_FILE_SIZE)
			{
				char* buffer = static_cast<char*>(malloc(static_cast<size_t>(dwSize + 1ULL)));
				if (buffer != nullptr)
				{
					DWORD dwRead = 0;
					BOOL iRet = ::ReadFile(hFile, buffer, dwSize, &dwRead, nullptr);
					if (iRet)
					{
						::CloseHandle(hFile);
						*(buffer + dwRead) = '\0';
						return buffer;
					}
					else
					{
						free(buffer);
					}
				}
			}
			::CloseHandle(hFile);
		}

		return nullptr;
	}
	/*電子網資源の大きさ取得*/
	bool GetInternetResourceSize(const char* pzUrl, ULARGE_INTEGER& size)
	{
		CComPtr<IStream> pStream;
		HRESULT hr = ::URLOpenBlockingStreamA(nullptr, pzUrl, &pStream, 0, nullptr);

		if (hr == S_OK)
		{
			STATSTG stat;
			hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
			if (hr == S_OK)
			{
				size = stat.cbSize;
				return true;
			}
		}
		return false;
	}
	/*URLからファイル名取得*/
	std::string GetFileNameFromUrl(const std::string& url)
	{
		size_t pos = url.find_last_of("/");
		if (pos != std::string::npos)
		{
			return url.substr(pos + 1);
		}

		return std::string();
	}
	/*電子網資源の保存*/
	bool SaveInternetResourceToFile(const char* pzUrl, const char* pzFolder, const char* pzFileName, unsigned long nMinFileSize, bool bFolderCreation)
	{
		std::string strFileName = pzFileName == nullptr ? GetFileNameFromUrl(pzUrl) : strchr(pzFileName, '/') == nullptr ? pzFileName : GetFileNameFromUrl(pzFileName);
		if (!strFileName.empty())
		{
			std::string strFilePath = pzFolder + strFileName;

			if (!::PathFileExistsA(strFilePath.c_str()))
			{
				ULARGE_INTEGER size{};
				bool bRet = GetInternetResourceSize(pzUrl, size);
				if (bRet)
				{
					if (size.LowPart > nMinFileSize)
					{
						if (bFolderCreation)
						{
							::CreateDirectoryA(pzFolder, nullptr);
						}

						HRESULT hr = ::URLDownloadToFileA(nullptr, pzUrl, strFilePath.c_str(), 0, nullptr);
						if (hr == S_OK)
						{
							WriteMessage(std::string(pzUrl).append(" success").c_str());
							return true;
						}
						else
						{
							WriteMessage(std::string(pzUrl).append(" failed").c_str());
						}
					}

				}
			}
			else
			{
				WriteMessage(std::string(strFileName).append(" already exists.").c_str());
				return true;
			}
		}
		else
		{
			WriteMessage("File path invalid.");
		}

		return false;
	}
	/*メモリのファイル出力*/
	bool WriteStringToFile(const std::string& strData, const char* pzFilePath)
	{
		BOOL iRet = 0;

		if (pzFilePath != nullptr)
		{
			HANDLE hFile = ::CreateFileA(pzFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				::SetFilePointer(hFile, NULL, nullptr, FILE_END);

				DWORD bytesWrite = 0;
				iRet = ::WriteFile(hFile, strData.data(), static_cast<DWORD>(strData.size()), &bytesWrite, nullptr);
				::CloseHandle(hFile);
			}
		}
		return iRet > 0;
	}
	/*保存処理メッセージ出力*/
	void SaveStringToFile(const std::string& strData, const char* pzFilePath)
	{
		if (pzFilePath != nullptr)
		{
			std::string strFileName = GetFileNameFromUrl(pzFilePath);
			if (!strFileName.empty())
			{
				bool bRet = WriteStringToFile(strData, pzFilePath);
				if (bRet)
				{
					WriteMessage(std::string(strFileName).append(" success.").c_str());
				}
				else
				{
					WriteMessage(std::string(strFileName).append(" failed.").c_str());
				}
			}
		}

	}
	/*ファイル有無確認*/
	bool DoesFilePathExist(const char* pzFilePath)
	{
		BOOL iRet = ::PathFileExistsA(pzFilePath);
		if (iRet)
		{
			std::string strFileName = GetFileNameFromUrl(pzFilePath);
			WriteMessage(std::string(strFileName).append(" already exists.").c_str());
		}

		return iRet == TRUE;
	}

	char* EncodeUri(const char* src)
	{
		if (src == nullptr)return nullptr;

		const char HexDigits[] = "0123456789ABCDEF";

		size_t nSrcLen = strlen(src);
		char* pResult = static_cast<char*>(malloc(nSrcLen * 3 + 1LL));
		if (pResult == nullptr)return nullptr;

		char* pPos = pResult;
		size_t nDstLen = 0;

		for (size_t i = 0; i < nSrcLen; ++i)
		{
			char p = *(src + i);

			/*RFC 3986 Unreserved Characters*/
			if ((p >= 'A' && p <= 'Z') || (p >= 'a' && p <= 'z') || (p >= '0' && p <= '9') ||
				p == '-' || p == '_' || p == '.' || p == '~')
			{
				*pPos++ = p;
				++nDstLen;
			}
			else
			{
				*pPos++ = '%';
				*pPos++ = HexDigits[p >> 4];
				*pPos++ = HexDigits[p & 0x0f];
				nDstLen += 3;
			}
		}

		char* pTemp = static_cast<char*>(realloc(pResult, nDstLen + 1LL));
		if (pTemp != nullptr)
		{
			pResult = pTemp;
		}
		*(pResult + nDstLen) = '\0';

		return pResult;
	}

	char* DecodeUri(const char* src)
	{
		if (src == nullptr)return nullptr;

		size_t nSrcLen = strlen(src);
		char* pResult = static_cast<char*>(malloc(nSrcLen + 1LL));
		if (pResult == nullptr)return nullptr;

		size_t nPos = 0;
		size_t nLen = 0;
		char* pp = const_cast<char*>(src);

		for (;;)
		{
			char* p = strchr(pp, '%');
			if (p == nullptr)
			{
				nLen = nSrcLen - (pp - src);
				memcpy(pResult + nPos, pp, nLen);
				nPos += nLen;
				break;
			}

			nLen = p - pp;
			memcpy(pResult + nPos, pp, nLen);
			nPos += nLen;
			pp = p + 1;

			char pzBuffer[3]{};
			memcpy(pzBuffer, pp, 2);
			if (isxdigit(static_cast<unsigned char>(pzBuffer[0])) && isxdigit(static_cast<unsigned char>(pzBuffer[1])))
			{
				*(pResult + nPos) = static_cast<char>(strtol(pzBuffer, nullptr, 16));
				++nPos;
				pp += 2;
			}

		}

		char* pTemp = static_cast<char*>(realloc(pResult, nPos + 1LL));
		if (pTemp != nullptr)
		{
			pResult = pTemp;
		}
		*(pResult + nPos) = '\0';

		return pResult;
	}

	/*指定階層のファイル・フォルダ一覧生成*/
	bool CreateFilePathList(const char* pwzFolderPath, const char* pwzFileExtension, std::vector<std::string>& paths)
	{
		if (pwzFolderPath == nullptr)return false;

		std::string strParent = pwzFolderPath;
		strParent += "\\/";
		std::string strPath = strParent + '*';
		if (pwzFileExtension != nullptr)
		{
			strPath += pwzFileExtension;
		}

		WIN32_FIND_DATAA sFindData;
		std::vector<std::string> strNames;

		HANDLE hFind = ::FindFirstFileA(strPath.c_str(), &sFindData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			if (pwzFileExtension != nullptr)
			{
				do
				{
					/*ファイル一覧*/
					if (!(sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						strNames.push_back(sFindData.cFileName);
					}
				} while (::FindNextFileA(hFind, &sFindData));
			}
			else
			{
				do
				{
					/*フォルダ一覧*/
					if ((sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						if (strcmp(sFindData.cFileName, ".") != 0 && strcmp(sFindData.cFileName, "..") != 0)
						{
							strNames.push_back(sFindData.cFileName);
						}
					}
				} while (::FindNextFileA(hFind, &sFindData));
			}

			::FindClose(hFind);
		}

		/*名前順整頓不可*/

		for (const std::string &str : strNames)
		{
			paths.push_back(strParent + str);
		}

		return paths.size() > 0;
	}

	/*======================== End of Copy ========================*/

	/*版情報取得*/
	bool GetMnsgVersion()
	{
		std::string strUrl = "https://minasigo-no-shigoto-web-r-server.orphans-order.com/mnsg/user/getVersion";
		std::string strFolder = CreateWorkFolder("Auth");
		std::string strFileName = "getVersion.json";

		bool bRet = SaveInternetResourceToFile(strUrl.c_str(), strFolder.c_str(), strFileName.c_str(), 0, false);
		if (bRet)
		{
			std::string strFile = strFolder + strFileName;
			char* buffer = LoadExistingFile(strFile.c_str());
			if (buffer != nullptr)
			{
				char element[256]{};

				GetJsonElementValue(buffer, "resourceVersion", element, sizeof(element));
				g_MnsgVersion.strResourceVersion = element;

				GetJsonElementValue(buffer, "masterVersion", element, sizeof(element));
				g_MnsgVersion.strMasterVersion = element;

				GetJsonElementValue(buffer, "clientVersion", element, sizeof(element));
				g_MnsgVersion.strClientVersion = element;

				free(buffer);

				return true;
			}
		}

		return false;
	}

	/*認証情報読み取り*/
	void ReadAuthorityFiles()
	{
		std::string strFolder = GetBaseFolderPath() + "Auth\\/";
		std::string strFile;
		char* buffer = nullptr;
		char element[256]{};
		g_Auth = Authorisation();

		strFile = strFolder + "JsonAsset.json";
		buffer = LoadExistingFile(strFile.c_str());
		if (buffer != nullptr)
		{
			GetJsonElementValue(buffer, "consumerKey", element, sizeof(element));
			g_Auth.strConsumerKey = element;

			GetJsonElementValue(buffer, "consumerSecret", element, sizeof(element));
			g_Auth.strConsumerSecret = element;

			free(buffer);
		}
		strFile = strFolder + "getDmmAccessToken.json";
		buffer = LoadExistingFile(strFile.c_str());
		if (buffer != nullptr)
		{
			GetJsonElementValue(buffer, "userId", element, sizeof(element));
			g_Auth.strUserId = element;

			GetJsonElementValue(buffer, "token", element, sizeof(element));
			g_Auth.strToken = element;

			GetJsonElementValue(buffer, "secret", element, sizeof(element));
			g_Auth.strSecret = element;

			free(buffer);
		}

	}

	/*----------------------------------------  認証生成用ここから  ----------------------------------------*/

	char* MinasigonReplace(const char* src)
	{
		if (src == nullptr)return nullptr;

		const char HexDigits[] = "0123456789ABCDEF";

		size_t nSrcLen = strlen(src);
		char* pResult = static_cast<char*>(malloc(nSrcLen * 3 + 1LL));
		if (pResult == nullptr)return nullptr;

		char* pPos = pResult;
		size_t nDstLen = 0;

		for (size_t i = 0; i < nSrcLen; ++i)
		{
			char p = *(src + i);

			if (p == '(' || p == ')' || p == '$' || p == '!' || p == '*' || p == '\'')
			{
				*pPos++ = '%';
				*pPos++ = HexDigits[p >> 4];
				*pPos++ = HexDigits[p & 0x0f];
				nDstLen += 3;
			}
			else
			{
				*pPos++ = p;
				++nDstLen;
			}
		}

		char* pTemp = static_cast<char*>(realloc(pResult, nDstLen + 1LL));
		if (pTemp != nullptr)
		{
			pResult = pTemp;
		}
		*(pResult + nDstLen) = '\0';

		return pResult;
	}

	std::string MinasigonEncode(const char* src)
	{
		std::string strEncoded;
		if (src != nullptr)
		{
			char* pUriEncoded = EncodeUri(src);
			if (pUriEncoded != nullptr)
			{
				char* pMinasigonReplaced = MinasigonReplace(pUriEncoded);
				if (pMinasigonReplaced != nullptr)
				{
					strEncoded = pMinasigonReplaced;
					free(pMinasigonReplaced);
				}
				free(pUriEncoded);
			}
		}

		return strEncoded;
	}

	std::string GetNonce()
	{
		static bool bInitialised = false;
		if (!bInitialised)
		{
			::srand(static_cast<unsigned int>(GetUnixTime()));
			bInitialised = true;
		}

		double x = static_cast<double>(::rand()) / static_cast<double>(RAND_MAX);
		/*JavaScript MAX_SAFE_INTEGER*/
		long long llNonce = static_cast<long long>(::floor(x * 9007199254740991));

		return std::to_string(llNonce);
	}

	std::string EncodeSignature(const char* pzMethod, const char* pzUrl, const char* pzParameter)
	{
		std::string strSignature;

		if (pzMethod != nullptr && pzUrl != nullptr)
		{
			std::string strEncodedUrl = MinasigonEncode(pzUrl);
			std::string strEncodedParameter = MinasigonEncode(pzParameter);

			std::string strMsg;
			strMsg += pzMethod;
			strMsg += '&';
			strMsg += strEncodedUrl;
			strMsg += '&';
			strMsg += strEncodedParameter;

			std::string strKey;
			strKey += g_Auth.strConsumerSecret;
			strKey += '&';
			strKey += g_Auth.strSecret;

			std::string strHex = hmac<SHA256>(strMsg, strKey);
			std::string strByte;
			for (size_t i = 0; i < strHex.size(); i += 2)
			{
				strByte.push_back(static_cast<unsigned char>(strtol(strHex.substr(i, 2).c_str(), nullptr, 16)));
			}
			strSignature = base64::to_base64(strByte);
		}

		return strSignature;
	}

	std::string ChainParameters(const std::string &strNonce, const std::string &strTimeStamp)
	{
		std::string strParameter;
		strParameter.reserve(512);

		strParameter += "oauth_consumer_key=";
		strParameter += g_Auth.strConsumerKey;
		strParameter += "&oauth_nonce=";
		strParameter += strNonce;
		strParameter += "&oauth_signature_method=HMAC-SHA256";
		strParameter += "&oauth_timestamp=";
		strParameter += strTimeStamp;
		strParameter += "&oauth_token=";
		strParameter += g_Auth.strToken;
		strParameter += "&xoauth_requestor_id=";
		strParameter += g_Auth.strUserId;

		return strParameter;
	}

	std::wstring CreateAuthorisation(const char* pzMethod, const char* pzUrl)
	{
		std::string strAuthorisation;
		strAuthorisation.reserve(512);

		std::string strNonce = GetNonce();
		std::string strTimeStamp = std::to_string(GetUnixTime());
		std::string strSignature = EncodeSignature(pzMethod, pzUrl, ChainParameters(strNonce, strTimeStamp).c_str());

		strAuthorisation += "Authorization: OAuth realm=\"Users\" oauth_token=\"";
		strAuthorisation += g_Auth.strToken;
		strAuthorisation += "\" xoauth_requestor_id=\"";
		strAuthorisation += g_Auth.strUserId;
		strAuthorisation += "\" oauth_consumer_key=\"";
		strAuthorisation += g_Auth.strConsumerKey;
		strAuthorisation += "\" oauth_signature_method=\"HMAC-SHA256\" oauth_nonce=\"";
		strAuthorisation += strNonce;
		strAuthorisation += "\" oauth_timestamp=\"";
		strAuthorisation += strTimeStamp;
		strAuthorisation += "\" oauth_signature=\"";
		strAuthorisation += strSignature;
		strAuthorisation += "\"\r\n";

		return WidenUtf8(strAuthorisation);
	}
	/*----------------------------------------  認証生成用ここまで  ----------------------------------------*/

	std::wstring CreateAppVersion()
	{
		std::string strVersion;
		strVersion.reserve(128);
		strVersion += "X-Mnsg-App-Version: {\"clientVersion\":\"";
		strVersion += g_MnsgVersion.strClientVersion;
		strVersion += "\",\"masterVersion\":\"";
		strVersion += g_MnsgVersion.strMasterVersion;
		strVersion += "\",\"resourceVersion\":\"";
		strVersion += g_MnsgVersion.strResourceVersion;
		strVersion += "\"}\r\n";

		return WidenUtf8(strVersion);
	}

	std::wstring CreateRequestHeader(const char* pzMethod, const char* pzUrl)
	{
		std::wstring wstrHeader;
		std::wstring wstrAuth = CreateAuthorisation(pzMethod, pzUrl);
		std::wstring wstrVersion = CreateAppVersion();

		wstrHeader += L"Accept: application/json;charset=UTF-8\r\n";
		wstrHeader += L"Content-Type: application/json;charset=UTF-8\r\n";
		wstrHeader += wstrAuth;
		wstrHeader += L"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/127.0.0.0 Safari/537.36 Edg/127.0.0.0\r\n";
		wstrHeader += wstrVersion;

		return wstrHeader;
	}

	/*HTTP POST要求*/
	bool RequestHttpPost(const std::wstring &wstrUrl, const std::wstring &wstrHeader, const std::string &strPayload, ResponseData &r)
	{
		bool bRet = false;

		CWinHttpSession* pSession = new CWinHttpSession();
		if (pSession != nullptr)
		{
			bRet = pSession->Open();
			if (bRet)
			{
				bRet = pSession->RequestPost(wstrUrl.c_str(), wstrHeader.c_str(), strPayload.c_str(), static_cast<DWORD>(strPayload.size()), r);
			}
			delete pSession;
		}

		return bRet;
	}

	/*Example: {"path":"adv/image/character/216201/image/106.jpg","quality":3} => {data:"U2Fs..."}*/
	std::string CreateGetStoryResourcePayload(const std::string& strPath)
	{
		if (g_pMainWindow != nullptr)
		{
			std::string strRaw = "{\"path\":\"";
			strRaw += strPath;
			strRaw += "\", \"quality\":3}";

			std::wstring wstrEncrypted = g_pMainWindow->ExecuteEncryptDataFunctionOnWebPage(WidenUtf8(strRaw), WidenUtf8(g_Auth.strToken));
			if (!wstrEncrypted.empty())
			{
				std::string strPayload = "{\"data\":\"" + NarrowUtf8(wstrEncrypted) + "\"}";
				return strPayload;
			}
		}

		return std::string();
	}
	/*getStoryResource要求*/
	std::string RequestGetStoryResource(const std::string& strPath)
	{
		if (g_pMainWindow == nullptr)return std::string();

		std::wstring wstrUrl = L"https://minasigo-no-shigoto-web-r-server.orphans-order.com/mnsg/story/getStoryResource";
		std::wstring wstrHeader = CreateRequestHeader("POST", NarrowUtf8(wstrUrl).c_str());
		std::string strPayload = CreateGetStoryResourcePayload(strPath);
		ResponseData r;

		bool bRet = RequestHttpPost(wstrUrl, wstrHeader, strPayload, r);
		if (bRet)
		{
			if (wcsstr(r.header.c_str(), L"HTTP/1.1 200 OK") && r.content.size() > 2)
			{
				std::wstring wstrBody = WidenUtf8(r.content.substr(1, r.content.size() - 2));
				std::wstring wstrDecrypted = g_pMainWindow->ExecuteDecryptDataFunctionOnWebPage(wstrBody, WidenUtf8(g_Auth.strToken));
				if (wcsstr(wstrDecrypted.c_str(), L"md5") == nullptr)
				{
					printf("Response data:\r\n%S\r\n", wstrDecrypted.c_str());
				}
				return NarrowUtf8(wstrDecrypted);
			}
			else
			{
				printf("Header content:\r\n%S", r.header.c_str());
				printf("Response data:\r\n%s\r\n", r.content.c_str());
			}
		}
		else
		{
			printf("WinHttp failed; function: %s, code: %ld\r\n", r.error.c_str(), r.ulErrorCode);
		}

		return std::string();
	}

	/*副経路算出*/
	std::string GetSubPath(const char* hash)
	{
		if (hash == nullptr)return std::string();

		std::string strPath;

		std::string strHash = hash;
		if (strHash.size() > 8)
		{
			char buffer[2]{};
			buffer[0] = hash[0];
			if (isxdigit(static_cast<unsigned char>(buffer[0])))
			{
				long lHex = strtol(buffer, nullptr, 16);
				if (lHex < 0x04)
				{
					strPath = '/' + strHash.substr(0, 2) + '/' + strHash.substr(4, 2) + '/';
				}
				else if (lHex < 0x08 && lHex >= 0x04)
				{
					strPath = '/' + strHash.substr(2, 2) + '/' + strHash.substr(6, 2) + '/' + strHash.substr(0, 2) + '/';
				}
				else if (lHex < 0x0c && lHex >= 0x08)
				{
					strPath = '/' + strHash.substr(4, 2) + '/' + strHash.substr(0, 2) + '/' + strHash.substr(6, 2) + '/' + strHash.substr(2, 2) + '/';
				}
				else
				{
					strPath = '/' + strHash.substr(6, 2) + '/' + strHash.substr(2, 2) + '/' + strHash.substr(4, 2) + '/' + strHash.substr(0, 2) + '/';
				}
			}
		}

		return strPath;
	}
	/*経路生成*/
	std::string CreateResourcePath(const ResourcePath& sResourcePath)
	{
		size_t nPos = sResourcePath.strFilePath.find_last_of(".");
		if (nPos == std::string::npos)return std::string();

		std::string strRawFileName = sResourcePath.strFilePath.substr(0, nPos);
		std::string strExtension = sResourcePath.strFilePath.substr(nPos);

		std::string strDir1 = md5(sResourcePath.strFilePath);
		std::string strDir2 = GetSubPath(md5(strRawFileName).c_str());

		std::string strUrl = "https://minasigo-no-shigoto-pd-c-res.orphans-order.com/";
		strUrl += g_MnsgVersion.strResourceVersion;
		strUrl += '/';
		strUrl += strDir1;
		strUrl += strDir2;
		strUrl += sResourcePath.strMd5;
		strUrl += strExtension;

		return strUrl;
	}
	/*資源経路情報読み取り*/
	void ReadResourcePathValue(char* src, ResourcePath& r)
	{
		char buffer[256]{};
		bool bRet = false;

		bRet = GetJsonElementValue(src, "path", buffer, sizeof(buffer));
		if (bRet)
		{
			r.strFilePath = buffer;
		}

		bRet = GetJsonElementValue(src, "md5", buffer, sizeof(buffer));
		if (bRet)
		{
			r.strMd5 = buffer;
		}
	}
	/*一覧新規作成*/
	bool CreatePathListCsv(const char* pzFilePath)
	{
		if (pzFilePath != nullptr)
		{
			HANDLE hFile = ::CreateFileA(pzFilePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				std::string strHeader = "\"md5\",\"path\"\r\n";
				DWORD dwWritten = 0;
				BOOL iRet = ::WriteFile(hFile, strHeader.c_str(), static_cast<DWORD>(strHeader.size()), &dwWritten, nullptr);
				::CloseHandle(hFile);
				return iRet > 0;
			}
		}

		return false;
	}
	/*一覧確認*/
	bool IsWritten(const char* pzFilePath, const char* pzKey)
	{
		bool bRet = false;
		char* buffer = LoadExistingFile(pzFilePath);
		if (buffer != nullptr)
		{
			if (strstr(buffer, pzKey) != nullptr)
			{
				bRet = true;
			}
			free(buffer);
		}

		return bRet;
	}
	/*一覧書き込み*/
	bool WritePathToListCsv(const char* pzFilePath, const ResourcePath& r)
	{
		HANDLE hFile = ::CreateFileA(pzFilePath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			::SetFilePointer(hFile, NULL, nullptr, FILE_END);
			std::string strWritten = r.strMd5 + "," + r.strFilePath + "\r\n";
			DWORD dwWritten = 0;
			BOOL iRet = ::WriteFile(hFile, strWritten.c_str(), static_cast<DWORD>(strWritten.size()), &dwWritten, nullptr);
			::CloseHandle(hFile);
			return iRet > 0;
		}
		return false;
	}
	/*一覧更新*/
	bool UpdatePathListCsv(const char* pzFilePath, const ResourcePath& r)
	{
		if (!::PathFileExistsA(pzFilePath))
		{
			if (!CreatePathListCsv(pzFilePath))return false;
		}

		if (IsWritten(pzFilePath, r.strMd5.c_str()))
		{
			return true;
		}
		return WritePathToListCsv(pzFilePath, r);
	}

	/*資源経路要求・保存*/
	bool RequestResourcePathAndSaveToFile(const std::string& strFilePath, const std::string& strRelativeUri)
	{
		if (IsWritten(strFilePath.c_str(), strRelativeUri.c_str()))
		{
			WriteMessage(std::string(strRelativeUri).append(" already requested.").c_str());
			return true;
		}

		std::string strResult = RequestGetStoryResource(strRelativeUri);
		if (!strResult.empty())
		{
			ResourcePath r;
			ReadResourcePathValue(const_cast<char*>(strResult.c_str()), r);
			if (r.strMd5.empty() && strstr(strResult.c_str(), "story master not found") != nullptr)
			{
				WriteMessage(std::string(strRelativeUri).append(" skipped.").c_str());
				return true;
			}
			else
			{
				if (UpdatePathListCsv(strFilePath.c_str(), r))
				{
					WriteMessage(std::string(strRelativeUri).append(" successfully requested.").c_str());
					return true;
				}
			}

		}
		else
		{
			WriteMessage(std::string(strRelativeUri).append(" failed to request.").c_str());
		}

		return false;
	}
	/*脚本経路要求*/
	bool RequestScenarioPath(const std::string& strResourcePath)
	{
		std::string strFilePath = CreateWorkFolder("ResourcePath") + "textfiles.txt";
		std::string strRelativeUri = "adv/textfiles/story/" + strResourcePath + ".txt";

		return RequestResourcePathAndSaveToFile(strFilePath, strRelativeUri);
	}
	/*ResourcePathR18探索*/
	void SearchResourcePathR18(char* src, std::vector<std::string>& resourcePaths)
	{
		if (src == nullptr)return;

		char* p = nullptr;
		char* pp = src;
		const char key[] = "resourcePathR18";
		for (;;)
		{
			char buffer[256]{};

			p = strstr(pp, key);
			if (p == nullptr)break;

			GetJsonElementValue(pp, key, buffer, sizeof(buffer));
			if (strlen(buffer))
			{
				resourcePaths.push_back(buffer);
			}
			pp = p + strlen(key);
		}
	}
	/*脚本ファイル経路取得*/
	void GetScenarioPaths()
	{
		std::string strFile = GetBaseFolderPath() + "MasterData\\/StoryMasterData.txt";
		char* buffer = LoadExistingFile(strFile.c_str());
		if (buffer != nullptr)
		{
			std::vector<std::string> resourcePaths;
			SearchResourcePathR18(buffer, resourcePaths);
			free(buffer);
			int iBlank = 0;
			for (const std::string &str : resourcePaths)
			{
				bool bRet = RequestScenarioPath(str);
				bRet ? iBlank = 0 : ++iBlank;
				if (iBlank > 1)break;
			}
		}
	}
	/*story_MasterData取得*/
	bool GetStoryMasterData()
	{
		std::string strUrl = "https://minasigo-no-shigoto-web-r-server.orphans-order.com/mnsg/story/getMasterData";
		std::string strFolder = CreateWorkFolder("MasterData");
		std::string strFileName = "StoryMasterData.mp";
		return SaveInternetResourceToFile(strUrl.c_str(), strFolder.c_str(), strFileName.c_str(), 0, false);
	}

	/*一覧復元*/
	bool ReadPathListCsv(char* src, std::vector<ResourcePath>& ResourcePaths)
	{
		char* p = nullptr;
		char* pp = src;
		int iCount = 0;
		char buffer[256]{};
		size_t nLen = 0;

		p = strstr(pp, "\r\n");
		if (p == nullptr)return false;
		p += 2;

		for (;; ++iCount)
		{
			ResourcePath r;

			pp = strchr(p, ',');
			if (pp == nullptr)break;
			nLen = pp - p;
			if (nLen > sizeof(buffer))break;
			memcpy(buffer, p, nLen);
			*(buffer + nLen) = '\0';
			r.strMd5 = buffer;
			p = pp + 1;

			pp = strstr(p, "\r\n");
			if (pp == nullptr)break;
			nLen = pp - p;
			if (nLen > sizeof(buffer))break;
			memcpy(buffer, p, nLen);
			*(buffer + nLen) = '\0';
			r.strFilePath = buffer;
			p = pp + 2;

			ResourcePaths.push_back(r);
		}

		return iCount > 0;
	}
	/*脚本取得*/
	void DownloadScenarios()
	{
		std::string strFile = GetBaseFolderPath() + "ResourcePath\\/textfiles.txt";
		char* buffer = LoadExistingFile(strFile.c_str());
		if (buffer != nullptr)
		{
			std::vector<ResourcePath> ResourcePaths;
			ReadPathListCsv(buffer, ResourcePaths);
			free(buffer);

			for (const ResourcePath &r : ResourcePaths)
			{
				std::string strUrl = CreateResourcePath(r);

				size_t nPos = r.strFilePath.rfind('/');
				if (nPos == std::string::npos)continue;

				std::string strFileName = r.strFilePath.substr(nPos + 1);
				nPos = strFileName.rfind('.');
				if (nPos == std::string::npos)continue;
				std::string strFolderName = strFileName.substr(0, nPos);

				std::string strFolderPath = CreateWorkFolder("Resource") + strFolderName + "\\/";
				SaveInternetResourceToFile(strUrl.c_str(), strFolderPath.c_str(), strFileName.c_str(), 0, true);
			}
		}
	}

	/*要素区切り位置探索*/
	char* FindFirstSeparation(char* src)
	{
		const char ref[] = ",\r\n\t";

		for (char* p = src; p != nullptr; ++p)
		{
			for (size_t i = 0; i < sizeof(ref); ++i)
			{
				if (*p == ref[i])
				{
					return p;
				}
			}
		}

		return nullptr;
	}

	/*脚本記載資源名探索*/
	void FindScenarioResourceName(char* src, std::vector<std::string> &resourceNames)
	{
		char* p = nullptr;
		char* pp = src;
		size_t nLen = 0;

		const char key1[] = "playvoice,1,";
		size_t nKey1Len = strlen(key1);
		const char key2[] = "sprite,";
		size_t nKey2Len = strlen(key2);

		/*音声*/
		for (;;)
		{
			p = strstr(pp, key1);
			if (p == nullptr)break;
			p += nKey1Len;

			pp = FindFirstSeparation(p);
			if (pp == nullptr)break;

			nLen = pp - p;
			char* buffer = static_cast<char*>(malloc(nLen + 1LL));
			if (buffer == nullptr)break;
			memcpy(buffer, p, nLen);
			*(buffer + nLen) = '\0';
			resourceNames.push_back(buffer);
			free(buffer);
		}

		pp = src;

		/*画像*/
		for (;;)
		{
			p = strstr(pp, key2);
			if (p == nullptr)break;
			p += nKey2Len;

			pp = FindFirstSeparation(p);
			if (pp == nullptr)break;

			nLen = pp - p;
			char* buffer = static_cast<char*>(malloc(nLen + 1LL));
			if (buffer == nullptr)break;
			memcpy(buffer, p, nLen);
			*(buffer + nLen) = '\0';

			/*演出効果除外*/
			if (strstr(buffer, "/ef/") == nullptr)
			{
				resourceNames.push_back(buffer);
			}

			free(buffer);
		}
	}

	/*脚本記載資源経路要求*/
	bool RequestScenarioResourcePath(const std::string&strFileName, const std::string& strResourcePath)
	{
		std::string strFilePath = CreateWorkFolder("ResourcePath") + strFileName;

		return RequestResourcePathAndSaveToFile(strFilePath, strResourcePath);
	}
	/*脚本記載資源経路取得*/
	void GetScenarioResourcePaths()
	{
		std::string strParentFolder = GetBaseFolderPath() + "Resource";
		std::vector<std::string> strFolders;
		CreateFilePathList(strParentFolder.c_str(), nullptr, strFolders);

		for (const std::string &strFolder : strFolders)
		{
			std::size_t nPos = strFolder.find_last_of("\\/");
			if (nPos == std::string::npos)continue;
			std::string strFileName = strFolder.substr(nPos + 1) + ".txt";
			std::string strFilePath = strFolder + "\\/" + strFileName;

			char* buffer = LoadExistingFile(strFilePath.c_str());
			if (buffer != nullptr)
			{
				std::vector<std::string> resourceRawNames;
				FindScenarioResourceName(buffer, resourceRawNames);
				free(buffer);

				int iBlank = 0;
				for (const std::string &resourceName : resourceRawNames)
				{
					bool bRet = RequestScenarioResourcePath(strFileName, resourceName);
					bRet ? iBlank = 0 : ++iBlank;
					if (iBlank > 1)return;
				}
			}
		}
	}
	/*脚本記載資源取得*/
	void DownloadScenarioResources()
	{
		std::string strParentFolder = GetBaseFolderPath() + "ResourcePath";
		std::vector<std::string> strTextPaths;
		CreateFilePathList(strParentFolder.c_str(), ".txt", strTextPaths);

		for (const std::string &strTextPath : strTextPaths)
		{
			if (strstr(strTextPath.c_str(), "textfiles.txt"))continue;

			size_t nPos = strTextPath.find_last_of("\\/");
			if (nPos == std::string::npos)continue;
			std::string strTextFileName = strTextPath.substr(nPos + 1);
			nPos = strTextFileName.find_last_of(".");
			if (nPos == std::string::npos)continue;
			std::string strFolderName = strTextFileName.substr(0, nPos);

			std::string strFolderPath = GetBaseFolderPath() + "Resource\\/" + strFolderName + "\\/";

			char* buffer = LoadExistingFile(strTextPath.c_str());
			if (buffer != nullptr)
			{
				std::vector<ResourcePath> ResourcePaths;
				ReadPathListCsv(buffer, ResourcePaths);
				free(buffer);

				for (const ResourcePath& r : ResourcePaths)
				{
					nPos = r.strFilePath.find_last_of("/");
					if (nPos == std::string::npos)continue;
					std::string strFileName = r.strFilePath.substr(nPos + 1);

					std::string strUrl = CreateResourcePath(r);
					SaveInternetResourceToFile(strUrl.c_str(), strFolderPath.c_str(), strFileName.c_str(), 0, false);
				}
			}
		}

	}

	/*Token受け渡し*/
	const std::wstring GetToken()
	{
		return WidenUtf8(g_Auth.strToken);
	}
	/*認証情報取得・主画面クラスポインタ格納*/
	void MnsgSetup(void* arg)
	{
		g_pMainWindow = static_cast<CMainWindow*>(arg);

		GetMnsgVersion();
		ReadAuthorityFiles();
	}

	/*
	* ============================================================================
	* To get resources by requesting "readStory" is given up, because they check if the account is qualified or not, 
	* and there seems no easy way to get it passed.
	* ============================================================================
	*/

	//bool RequestHttpPostAndWriteFile(std::wstring wstrUrl, std::wstring wstrHeader, std::string strPayload, const char* pzFilePath)
	//{
	//	ResponseData r;
	//	bool bRet = RequestHttpPost(wstrUrl, wstrHeader, strPayload, r);
	//	if (bRet)
	//	{
	//		if (bRet)
	//		{
	//			if (wcsstr(r.header.c_str(), L"HTTP/1.1 200 OK"))
	//			{
	//				SaveStringToFile(r.content, pzFilePath);
	//			}
	//			else
	//			{
	//				printf("Header content:\r\n%S", r.header.c_str());
	//				printf("Response data:\r\n%s\r\n", r.content.c_str());
	//				bRet = false;
	//			}
	//		}
	//		else
	//		{
	//			printf("WinHttp failed; function: %s, code: %ld\r\n", r.error.c_str(), r.ulErrorCode);
	//		}
	//	}

	//	return bRet;
	//}

	///*Example: {"storyId":130060311,"quality":3} => {data:"U2Fs..."}*/
	//std::string CreateReadStoryPayload(const std::string& strId)
	//{
	//	if (g_pMainWindow != nullptr)
	//	{
	//		std::string strRaw = "{\"storyId\":";
	//		strRaw += strId;
	//		strRaw += ", \"quality\":3}";

	//		std::wstring wstrEncrypted = g_pMainWindow->ExecuteEncryptDataFunctionOnWebPage(WidenUtf8(strRaw), WidenUtf8(g_Auth.strToken));
	//		if (!wstrEncrypted.empty())
	//		{
	//			std::string strPayload = "{\"data\":\"" + NarrowUtf8(wstrEncrypted) + "\"}";
	//			return strPayload;
	//		}
	//	}

	//	return std::string();
	//}
	///*readStory要求*/
	//bool RequestReadStory(const std::string& strId)
	//{
	//	std::string strFile = CreateWorkFolder("Episode") + strId + ".json";
	//	std::wstring wstrUrl = L"https://minasigo-no-shigoto-web-r-server.orphans-order.com/mnsg/story/readStory";
	//	std::wstring wstrHeader = CreateRequestHeader("POST", NarrowUtf8(wstrUrl).c_str());
	//	std::string strPayload = CreateReadStoryPayload(strId);

	//	return RequestHttpPostAndWriteFile(wstrUrl, wstrHeader, strPayload, strFile.c_str());
	//}
	///*StoryId候補絞り込み*/
	//void SearchStoryIds(char* src, std::vector<std::string>& storyIds)
	//{
	//	if (src == nullptr)return;

	//	char* p = nullptr;
	//	char* pp = src;
	//	const char key1[] = "storyId";
	//	const char key2[] = "resourcePathR18";

	//	for (;;)
	//	{
	//		char buffer[256]{};
	//		char buffer2[256]{};

	//		p = strstr(pp, key1);
	//		if (p == nullptr)break;

	//		GetJsonElementValue(pp, key1, buffer, sizeof(buffer));

	//		p = strstr(pp, key2);
	//		if (p == nullptr)break;

	//		GetJsonElementValue(pp, key2, buffer2, sizeof(buffer2));
	//		if (strlen(buffer2))
	//		{
	//			storyIds.push_back(buffer);
	//		}
	//		pp = p + strlen(key1);
	//	}
	//}
	///*資源経路情報格納*/
	//bool InsertResourcePathValue(char* src, std::vector<ResourcePath>& resource_path)
	//{
	//	ResourcePath r;
	//	ReadResourcePathValue(src, r);
	//	if (r.strFilePath.size() > 4)
	//	{
	//		resource_path.push_back(r);
	//		return true;
	//	}
	//	return false;
	//}
	///*資源経路情報探索*/
	//void SearchResourcePath(char* src, std::vector<ResourcePath>& resource_path)
	//{
	//	char* p = nullptr;
	//	char* q = nullptr;
	//	char* qq = nullptr;

	//	ExtractJsonArray(src, "resource", &p);
	//	if (p == nullptr)return;
	//	qq = p;

	//	for (;;)
	//	{
	//		q = strchr(qq, '{');
	//		if (q == nullptr)break;

	//		qq = strchr(q, '}');
	//		if (qq == nullptr)break;
	//		++qq;

	//		size_t len = qq - q;
	//		char* buffer = static_cast<char*>(malloc(len + 1LL));
	//		if (buffer == nullptr)break;
	//		memcpy(buffer, q, len);
	//		*(buffer + len) = '\0';
	//		InsertResourcePathValue(buffer, resource_path);
	//		free(buffer);

	//		qq = strchr(q, ',');
	//		if (qq == nullptr)break;
	//	}

	//	free(p);
	//}
	///*保存済み脚本ファイル検索*/
	//void FindEpisodeFiles(const char* pzFolder, std::vector<std::string>& episodes)
	//{
	//	if (pzFolder == nullptr)return;

	//	WIN32_FIND_DATAA sFindData;

	//	std::string strFile = std::string(pzFolder) + "*.json";
	//	HANDLE hFind = ::FindFirstFileA(strFile.c_str(), &sFindData);
	//	if (hFind != INVALID_HANDLE_VALUE)
	//	{
	//		do
	//		{
	//			if (!(sFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	//			{
	//				if (strlen(sFindData.cFileName) > 4)
	//				{
	//					episodes.push_back(sFindData.cFileName);
	//				}
	//			}
	//		} while (::FindNextFileA(hFind, &sFindData));
	//		::FindClose(hFind);
	//	}

	//}
	///*資源ファイル保存*/
	//void DownloadResources(const char* pzFolder, std::vector<ResourcePath> resource_path)
	//{
	//	std::string strFolder = CreateWorkFolder("Resource") + pzFolder + "\\/";
	//	for (size_t i = 0; i < resource_path.size(); ++i)
	//	{
	//		ResourcePath r = resource_path.at(i);
	//		std::string strUrl = CreateResourcePath(r);
	//		size_t nPos = r.strFilePath.find_last_of("/");
	//		if (nPos == std::string::npos)continue;

	//		std::string strFileName = r.strFilePath.substr(nPos + 1);
	//		SaveInternetResourceToFile(strUrl.c_str(), strFolder.c_str(), strFileName.c_str(), 0, true);
	//	}

	//}
	///*脚本記載資源取得*/
	//void GetEpisodeResources()
	//{
	//	std::string strFolder = GetBaseFolderPath() + "Decrypted\\/";

	//	std::vector<std::string> scenarios;

	//	FindEpisodeFiles(strFolder.c_str(), scenarios);

	//	for (size_t i = 0; i < scenarios.size(); ++i)
	//	{
	//		std::string strFile = strFolder + scenarios.at(i);
	//		char* buffer = LoadExistingFile(strFile.c_str());
	//		if (buffer != nullptr)
	//		{
	//			std::vector<ResourcePath> resource_path;
	//			SearchResourcePath(buffer, resource_path);
	//			DownloadResources(scenarios.at(i).substr(0, scenarios.at(i).size() - 4).c_str(), resource_path);
	//			free(buffer);
	//		}
	//	}

	//}
	///*資源経路要求*/
	//bool RequestResourcePathList(const std::string& strId)
	//{
	//	bool bRet = false;

	//	std::string strFile = CreateWorkFolder("Episode") + strId + ".json";
	//	if (DoesFilePathExist(strFile.c_str()))return true;

	//	bRet = RequestReadStory(strId);

	//	return bRet;
	//}

} //namespace minasigo
