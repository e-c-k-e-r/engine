#include <uf/utils/http/http.h>
#if UF_USE_CURL
#include <curl/curl.h>
#endif
#include <iostream>

namespace {
	size_t writeFunction(void *ptr, size_t size, size_t nmemb, uf::stl::string* data) {
    	data->append((char*) ptr, size * nmemb);
    	return size * nmemb;
	}
}

uf::Http uf::http::get( const uf::stl::string& url ) {
	uf::Http http;

#if UF_USE_CURL
	auto curl = curl_easy_init();
	if ( !curl ) return http;

	std::cout << curl << " " << url << std::endl;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
//	curl_easy_setopt(curl, CURLOPT_USERPWD, "user:pass");
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.42.0");
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &http.response);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &http.header);

	curl_easy_perform(curl);
	
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http.code);
	curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &http.elapsed);
	curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &http.effective);

	curl_easy_cleanup(curl);
	curl = NULL;
#endif
    return http;
}