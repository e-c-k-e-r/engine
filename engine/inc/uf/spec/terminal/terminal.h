#pragma once

#include <uf/config.h>
#include <uf/spec/universal.h>

namespace spec {
	namespace uni {
		class UF_API Terminal {
		protected:

		public:
			void UF_API_CALL clear();
			void UF_API_CALL setLocale();
			void UF_API_CALL hide();
			void UF_API_CALL show();
			inline void UF_API_CALL setVisible(bool visibility) { visibility ? this->show() : this->hide(); }
		};
	};

	class UF_API Terminal : public spec::uni::Terminal {
	protected:

	public:
		spec::uni::Terminal& UF_API_CALL getUniversal();

		void UF_API_CALL clear();
		void UF_API_CALL setLocale();
		void UF_API_CALL hide();
		void UF_API_CALL show();
		inline void UF_API_CALL setVisible(bool visibility) { visibility ? this->show() : this->hide(); }
	};
	extern UF_API spec::Terminal terminal;
}