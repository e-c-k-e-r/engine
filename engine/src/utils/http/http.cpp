#include <uf/utils/http/http.h>
#include <uf/utils/io/vfs.h>
#include <uf/utils/io/file.h>
#if UF_USE_CURL
#include <curl/curl.h>
#endif
#include <iostream>

namespace impl {
	pod::Mount createHttpMount( const uf::stl::string& uri, int priority ) {
		uf::stl::string prefix;
		uf::stl::string path;
		uf::io::splitUri( uri, prefix, path );

		return pod::Mount{
			.prefix = prefix,
			.path = path,
			.priority = priority,
			.exists = [prefix](const uf::stl::string& p) -> bool {
				uf::stl::string url = prefix + p;
				uf::Http http = uf::http::head(url);
				return ( http.code >= 200 && http.code < 300 );
			},
			.size = [prefix](const uf::stl::string& p) -> size_t {
				uf::stl::string url = prefix + p;
				uf::Http http = uf::http::head(url);
				return http.contentLength;
			},
			.mtime = [prefix](const uf::stl::string& p) -> size_t {
				uf::stl::string url = prefix + p;
				uf::Http http = uf::http::head(url);
				return http.mtime;
			},
			.read = [prefix](const uf::stl::string& p, uf::stl::vector<uint8_t>& buffer) -> bool {
				uf::stl::string url = prefix + p;

				uf::Http http = uf::http::get(url);
				if ( http.code < 200 || http.code >= 300 ) {
					UF_MSG_ERROR("HTTP Error {} on GET {}", http.code, url);
					return false;
				}

				buffer.assign(http.response.begin(), http.response.end());
				return true;
			},
			.write = [prefix](const uf::stl::string& p, const void* buffer, size_t size) -> size_t {
				uf::stl::string url = prefix + p;
				uf::Http http = uf::http::post(url, buffer, size);

				if ( http.code < 200 || http.code >= 300 ) {
					UF_MSG_ERROR("HTTP Error {} on POST {}", http.code, url);
					return 0;
				}
				return size;
			},
		};
	}
}

namespace {
	size_t writeFunction( void *ptr, size_t size, size_t nmemb, uf::stl::string* data ) {
		data->append((char*) ptr, size * nmemb);
		return size * nmemb;
	}

	uf::Http cURL( const uf::stl::string& url, const uf::stl::string& method = "GET", const void* data = nullptr, size_t size = 0 ) {
		uf::Http http;
	#if UF_USE_CURL
		auto curl = curl_easy_init();
		if ( !curl ) return http;

		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.42.0");
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);

		if ( method == "HEAD" ) {
			curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
		} else {
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &http.response);
		}

		if ( method == "POST" ) {
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			if (data) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) size);
		}

		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &http.header);

		curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http.code);
		curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &http.elapsed);
		curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &http.effective);

		double cl;
		if (curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl) == CURLE_OK && cl >= 0.0) {
			http.contentLength = (size_t)cl;
		} else {
			http.contentLength = 0;
		}

		long filetime = -1;
		if (curl_easy_getinfo(curl, CURLINFO_FILETIME, &filetime) == CURLE_OK && filetime >= 0) {
			http.mtime = (size_t)filetime;
		} else {
			http.mtime = 0;
		}

		curl_easy_cleanup(curl);
	#endif
		return http;
	}
}

uf::Http uf::http::get( const uf::stl::string& url ) {
	return ::cURL( url, "GET" );
}

uf::Http uf::http::head( const uf::stl::string& url ) {
	return ::cURL( url, "HEAD" );
}

uf::Http uf::http::post( const uf::stl::string& url, const void* data, size_t size ) {
	return ::cURL( url, "POST", data, size );
}

UF_VFS_MOUNT_CPP( impl::createHttpMount, "https://", -10 );
UF_VFS_MOUNT_CPP( impl::createHttpMount, "http://", -10 );