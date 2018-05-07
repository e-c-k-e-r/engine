#pragma once

#include <uf/config.h>

#include <string>

namespace ext {
	class UF_API Ncurses {
	protected:
		bool m_initialized;
		bool m_initializeOnCtor = false;
		void* m_window;
		int m_timeout;
	public:
	// 	C-tor/D-tor
		UF_API_CALL Ncurses();
		~Ncurses();
	// 	On-demand C-tor/D-tor
		void UF_API_CALL initialize();
		bool UF_API_CALL initialized();
		void UF_API_CALL terminate();
	// 	API (original named)
		
		void UF_API_CALL getYX(int& row, int& column);
		void UF_API_CALL getMaxYX(int& row, int& column);
		void UF_API_CALL move(int row, int column);
		void UF_API_CALL move(int toRow, int toColumn, int& row, int& column);

		void UF_API_CALL refresh();
		void UF_API_CALL clrToEol();
		void UF_API_CALL clrToBot();

		 int UF_API_CALL getCh();
		void UF_API_CALL delCh();
		void UF_API_CALL addCh( char c );
		void UF_API_CALL addStr(const char* c_str);
		void UF_API_CALL addStr(const std::string str);

		void UF_API_CALL attrOn(int att);
		void UF_API_CALL attrOff(int att);
		bool UF_API_CALL hasColors();
		bool UF_API_CALL startColor();
		void UF_API_CALL initPair( short id, short fore, short back );

	// 	API (different named)
		inline void UF_API_CALL clear(bool toBottom = true) { toBottom ? this->clrToBot() : this->clrToEol(); }
		inline  int UF_API_CALL getChar() { return this->getCh(); }
		inline void UF_API_CALL delChar() { this->delCh(); }
		inline void UF_API_CALL addChar( char c ) { this->addCh( c ); }
		inline void UF_API_CALL addString( const std::string& str ) { this->addStr( str.c_str() ); }
		inline void UF_API_CALL attribute(int att, bool on = true) { on ? this->attrOn(att) : this->attrOff(att); }
	};
	extern UF_API ext::Ncurses ncurses;
}