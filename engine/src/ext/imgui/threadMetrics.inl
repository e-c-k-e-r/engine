namespace {
	struct ThreadMetricsWindow {
		static const int HISTORY_SIZE = 120;

		struct ThreadHistory {
			float activeTime[HISTORY_SIZE] = { 0.0f };
			float idleTime[HISTORY_SIZE] = { 0.0f };
			float totalTime[HISTORY_SIZE] = { 0.0f };
			int offset = 0;
			bool filled = false;
		};

		uf::stl::unordered_map<uf::stl::string, ThreadHistory> histories;

		pod::Vector2ui size{400, 500};
		pod::Vector2ui position{32, 32};

		void Draw(const uf::stl::string& title, bool& open) {
			ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(ImVec2(position.x, position.y), ImGuiCond_FirstUseEver);

			if (!ImGui::Begin(title.c_str(), &open)) {
				ImGui::End();
				return;
			}

		#if UF_THREAD_METRICS
			auto metrics = uf::thread::collectStats();

			if (ImGui::BeginTable("ThreadMetricsTable", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
				ImGui::TableSetupColumn("Thread Info", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("History", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				for (auto& [name, stats] : metrics) {
					float active = std::get<0>(stats);
					float idle = std::get<1>(stats);
					float total = std::get<2>(stats);
					uint32_t tasks = std::get<3>(stats);

					auto& history = histories[name];

					history.activeTime[history.offset] = active;
					history.idleTime[history.offset] = idle;
					history.totalTime[history.offset] = total;
					history.offset++;
					if (history.offset >= HISTORY_SIZE) {
						history.offset = 0;
						history.filled = true;
					}

					int count = history.filled ? HISTORY_SIZE : (history.offset == 0 ? 1 : history.offset);
					float avgActive = 0.0f, avgIdle = 0.0f, avgTotal = 0.0f;

					for (int i = 0; i < count; ++i) {
						avgActive += history.activeTime[i];
						avgIdle += history.idleTime[i];
						avgTotal += history.totalTime[i];
					}
					avgActive /= count;
					avgIdle /= count;
					avgTotal /= count;

					ImGui::TableNextRow();

					ImGui::PushID(name.c_str());

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(name.c_str());
					ImGui::Text("Tasks: %u", tasks);
					ImGui::Text("Time:  %.2f ms", avgTotal);

					float utilization = (avgTotal > 0.0f) ? (avgActive / avgTotal) : 0.0f;
					char utilText[32];
					snprintf(utilText, sizeof(utilText), "Load: %.0f%%", utilization * 100.0f);

					if (utilization > 0.8f) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
					ImGui::ProgressBar(utilization, ImVec2(-FLT_MIN, 0), utilText);
					if (utilization > 0.8f) ImGui::PopStyleColor();

					ImGui::TableSetColumnIndex(1);

					float graphWidth = ImGui::GetContentRegionAvail().x;
					float max_val = 16.6f;

					char overlayActive[32];
					snprintf(overlayActive, sizeof(overlayActive), "Act: %.2fms", avgActive);
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
					ImGui::PlotHistogram("##Act", history.activeTime, HISTORY_SIZE, history.offset, overlayActive, 0.0f, max_val, ImVec2(graphWidth, 30));
					ImGui::PopStyleColor();

					char overlayIdle[32];
					snprintf(overlayIdle, sizeof(overlayIdle), "Idl: %.2fms", avgIdle);
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
					ImGui::PlotHistogram("##Idl", history.idleTime, HISTORY_SIZE, history.offset, overlayIdle, 0.0f, max_val, ImVec2(graphWidth, 20));
					ImGui::PopStyleColor();

					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		#else
			ImGui::TextColored(ImVec4(1,0,0,1), "UF_THREAD_METRICS is disabled!");
		#endif

			ImGui::End();
		}
	} threadMetrics;
}