#pragma once
#include <uf/config.h>

#include <string>

#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>

#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/string.h>

namespace uf {
	class UF_API IoStream {
	public:
		struct UF_API color_t {
			short id;
			short foreground;
			short background;
			std::string name;
		};
	
		class UF_API Color {
		public:
			class UF_API Manip1 {
			friend class uf::IoStream;
			friend class uf::IoStream::Color;
			protected:
				uf::IoStream& io;
				UF_API_CALL Manip1( uf::IoStream& );
			public:
				uf::IoStream& UF_API_CALL operator<<( const std::string& name );
			};
			class UF_API Manip2 {
			friend class uf::IoStream;
			friend class uf::IoStream::Color;
			protected:
				std::string name;

				UF_API_CALL Manip2( const std::string& = "" );
			};

			uf::IoStream::Color::Manip2 UF_API_CALL operator()( const std::string& = "" ) const;
			uf::IoStream& UF_API_CALL operator()( uf::IoStream&, const std::string& = "" ) const;
		};

		typedef std::unordered_map<std::string, uf::IoStream::color_t> color_container_t;
		static bool ncurses;
	protected:
		std::string m_currentColor;
		uf::IoStream::color_container_t m_registeredColors;
	public:
		UF_API_CALL IoStream();
		~IoStream();

		void UF_API_CALL initialize();
		void UF_API_CALL terminate();
		void UF_API_CALL clear(bool = true);
		void UF_API_CALL back();
		std::string getBuffer();
		std::vector<std::string> getHistory();
		
		char UF_API_CALL readChar(const bool& = true);
		std::string UF_API_CALL readString(const bool& = true);
		uf::String UF_API_CALL readUString(const bool& = true);
		void UF_API_CALL operator>> (bool&);
		void UF_API_CALL operator>> (short&);
		void UF_API_CALL operator>> (unsigned short&);
		void UF_API_CALL operator>> (char&);
		void UF_API_CALL operator>> (int&);
		void UF_API_CALL operator>> (unsigned int&);
		void UF_API_CALL operator>> (long&);
		void UF_API_CALL operator>> (unsigned long&);
		void UF_API_CALL operator>> (long long&);
		void UF_API_CALL operator>> (unsigned long long&);
		void UF_API_CALL operator>> (float&);
		void UF_API_CALL operator>> (double&);
		void UF_API_CALL operator>> (long double&);
		void UF_API_CALL operator>> (std::string&);
		void UF_API_CALL operator>> (uf::String&);
		std::istream& UF_API_CALL operator>> ( std::istream& );
		friend std::istream& UF_API_CALL operator>> ( std::istream&, uf::IoStream& );

		char UF_API_CALL writeChar(char);
		const std::string& UF_API_CALL writeString(const std::string&);
		const uf::String& UF_API_CALL writeUString(const uf::String&);
		uf::IoStream& UF_API_CALL operator<< (const bool&);
		uf::IoStream& UF_API_CALL operator<< (const short&);
		uf::IoStream& UF_API_CALL operator<< (const unsigned short&);
		uf::IoStream& UF_API_CALL operator<< (const char&);
		uf::IoStream& UF_API_CALL operator<< (const int&);
		uf::IoStream& UF_API_CALL operator<< (const unsigned int&);
		uf::IoStream& UF_API_CALL operator<< (const long&);
		uf::IoStream& UF_API_CALL operator<< (const unsigned long&);
		uf::IoStream& UF_API_CALL operator<< (const long long&);
		uf::IoStream& UF_API_CALL operator<< (const unsigned long long&);
		uf::IoStream& UF_API_CALL operator<< (const float&);
		uf::IoStream& UF_API_CALL operator<< (const double&);
		uf::IoStream& UF_API_CALL operator<< (const long double&);
		uf::IoStream& UF_API_CALL operator<< (const std::string&);
		uf::IoStream& UF_API_CALL operator<< (const uf::String&);
		uf::IoStream& UF_API_CALL operator<< (const char*);
		uf::IoStream& UF_API_CALL operator<< (const uf::Serializer& val);
		uf::IoStream& UF_API_CALL operator<< ( void* );
		uf::IoStream& UF_API_CALL operator<< ( std::ostream& );
		friend std::ostream& UF_API_CALL operator<< ( std::ostream&, uf::IoStream& );

		uf::IoStream::Color::Manip1 UF_API_CALL operator<< ( const uf::IoStream::Color& );
		uf::IoStream& UF_API_CALL operator<< ( const uf::IoStream::Color::Manip2& );
		
		std::string UF_API_CALL getColor();
		void UF_API_CALL setColor( const std::string& );
	};
	extern UF_API uf::IoStream iostream;
}