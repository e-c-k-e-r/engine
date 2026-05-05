namespace {
	struct ConsoleWindow {
		ImGuiTextFilter filter;
		int historyPos = -1;

		struct {
			bool automatic = true;
			bool bottom = false;
		} scroll;

		uf::stl::string commandBuf;

		pod::Vector2ui size{800, 600};
		pod::Vector2ui position{64, 32};

		void Draw( const uf::stl::string& title, bool& open ) {
			ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(ImVec2(position.x, position.y), ImGuiCond_FirstUseEver);

			if (!ImGui::Begin(title.c_str(), &open)) {
				ImGui::End();
				return;
			}

			if ( ImGui::BeginPopupContextItem() ) {
				if ( ImGui::MenuItem("Close Console") ) open = false;
				ImGui::EndPopup();
			}

			DrawToolbar();
			ImGui::Separator();
			DrawLog();
			ImGui::Separator();
			DrawInput();

			ImGui::End();
		}

	private:
		void DrawToolbar() {
			if ( ImGui::SmallButton("Clear") ) uf::console::clear();

			ImGui::SameLine();
			bool copyToClipboard = ImGui::SmallButton("Copy");

			if ( ImGui::Button("Options") ) {
				ImGui::OpenPopup("Options");
			}
			if ( ImGui::BeginPopup("Options") ) {
				ImGui::Checkbox("Auto-scroll", &scroll.automatic);
				ImGui::EndPopup();
			}

			ImGui::SameLine();
			filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);

			if ( copyToClipboard ) {
				ImGui::LogToClipboard();
				for ( auto& item : uf::console::log ) {
					if (filter.PassFilter(item.c_str())) ImGui::LogText("%s\n", item.c_str());
				}
				ImGui::LogFinish();
			}
		}

		void DrawLog() {
			const float footerReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
			ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerReserve), false, ImGuiWindowFlags_HorizontalScrollbar);

			if ( ImGui::BeginPopupContextWindow() ) {
				if ( ImGui::Selectable("Clear") ) uf::console::clear();
				ImGui::EndPopup();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

			for ( auto& item : uf::console::log ) {
				if ( !filter.PassFilter(item.c_str()) ) continue;

				ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				bool hasColor = false;

				if ( uf::string::matched(item, "^\\[ERROR\\]") ) {
					color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); hasColor = true;
				} else if ( uf::string::matched(item, "^\\> "))  {
					color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); hasColor = true;
				}

				if ( hasColor ) ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::TextUnformatted(item.c_str());
				if ( hasColor ) ImGui::PopStyleColor();
			}

			if ( scroll.bottom || (scroll.automatic && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ) {
				ImGui::SetScrollHereY(1.0f);
			}
			scroll.bottom = false;

			ImGui::PopStyleVar();
			ImGui::EndChild();
		}

		void DrawInput() {
			bool reclaimFocus = false;
			ImGuiInputTextFlags inputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

			if ( ImGui::InputText("Input", &commandBuf, inputTextFlags, &TextEditCallbackStub, (void*) this) ) {
				uf::string::trim( commandBuf );
				if ( !commandBuf.empty() ) {
					uf::console::execute(commandBuf);
					commandBuf.clear();
				}
				historyPos = -1;
				scroll.bottom = true;
				reclaimFocus = true;
			}

			ImGui::SetItemDefaultFocus();
			if ( reclaimFocus ) ImGui::SetKeyboardFocusHere(-1);
		}

		static int TextEditCallbackStub( ImGuiInputTextCallbackData* data ) {
			ConsoleWindow* console = (ConsoleWindow*) data->UserData;
			return console->TextEditCallback(data);
		}

		int TextEditCallback( ImGuiInputTextCallbackData* data ) {
			switch ( data->EventFlag ) {
				case ImGuiInputTextFlags_CallbackCompletion: {
					const char* end = data->Buf + data->CursorPos;
					const char* start = end;
					while ( start > data->Buf ) {
						const char c = start[-1];
						if (c == ' ' || c == '\t' || c == ',' || c == ';') break;
						start--;
					}

					uf::stl::string input{ start, end };
					uf::stl::vector<uf::stl::string> candidates;
					for ( auto& pair : uf::console::commands ) {
						if (pair.first.find(input) == 0) candidates.emplace_back(pair.first);
					}

					if ( candidates.empty() ) {
						UF_MSG_ERROR("No match for `{}`", input);
					} else if ( candidates.size() == 1 ) {
						data->DeleteChars((int)(start - data->Buf), (int)(end - start));
						data->InsertChars(data->CursorPos, candidates[0].c_str());
						data->InsertChars(data->CursorPos, " ");
					} else {
						int matchLen = (int)(end - start);
						while ( true ) {
							if ( matchLen >= candidates[0].size() ) break;
							char c = candidates[0][matchLen];
							bool allMatch = true;
							for ( size_t i = 1; i < candidates.size(); i++ ) {
								if ( matchLen >= candidates[i].size() || candidates[i][matchLen] != c ) {
									allMatch = false;
									break;
								}
							}
							if ( !allMatch ) break;
							matchLen++;
						}

						if ( matchLen > (int) (end - start) ) {
							data->DeleteChars((int)(start - data->Buf), (int)(end - start));
							data->InsertChars(data->CursorPos, candidates[0].substr(0, matchLen).c_str());
						} else {
							UF_MSG_DEBUG("Matches: {}", uf::string::join(candidates, " "));
						}
					}
				} break;

				case ImGuiInputTextFlags_CallbackHistory: {
					const int prevPos = historyPos;
					if ( data->EventKey == ImGuiKey_UpArrow ) {
						if ( historyPos == -1 ) historyPos = (int)uf::console::history.size() - 1;
						else if ( historyPos > 0 ) historyPos--;
					} else if ( data->EventKey == ImGuiKey_DownArrow ) {
						if ( historyPos != -1 && ++historyPos >= (int)uf::console::history.size() ) historyPos = -1;
					}

					if ( prevPos != historyPos ) {
						const char* historyStr = (historyPos >= 0) ? uf::console::history[historyPos].c_str() : "";
						data->DeleteChars(0, data->BufTextLen);
						data->InsertChars(0, historyStr);
					}
				} break;
			}
			return 0;
		}
	} consoleWindow;
}