#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "implot.h"

#include <SDL.h>

#include "harakiri.hh"

int main(int argc, char** argv)
{
  if (argc < 2) {
    printf("You need to specify a media file.\n");
    return -1;
  }

  MP4_Sosite_Chlen hui;
  if (load_mp4(hui, argv[1]) != 0)
    return -1;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

  SDL_Window* window = SDL_CreateWindow("Hararkiri: by Primaty?",
					SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
					// SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
					SDL_WINDOW_ALLOW_HIGHDPI
					);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	    // тяги

  ImPlot::CreateContext();

  // темная тема
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  Ebany_Time_series P_packets = {"P-packet size"};
  Ebany_Time_series I_packets = {"I-packet size"};

  Exponential_Huita smooth_p1(0.3, 0.01);
  Exponential_Huita smooth_p2(0.01, 0.01);
  std::vector<double> max_p_diff;

  Exponential_Huita smooth_i1(0.4, 0.17);
  Exponential_Huita smooth_i2(0.03, 0.17);
  std::vector<double> max_i_diff;

  bool show_demo_window = true;
  bool process_frames = true;
  bool video_done = false;

  ImVec4 clear_fine_color = ImVec4(0.45f, 0.85f, 0.60f, 1.00f);
  ImVec4 clear_anal_color = ImVec4(0.95f, 0.25f, 0.30f, 1.00f);

  double P_threshold = 0.012;
  double I_threshold = 0.015;

  std::vector<Anal_omaly> anals;

  // Main loop
  bool done = false;
  while (!done)
    {
      SDL_Event event;
      while (SDL_PollEvent(&event))
	{
	  ImGui_ImplSDL2_ProcessEvent(&event);
	  switch (event.type)
	    {
	    case SDL_QUIT:
	      done = true;
	      break;
	    case SDL_WINDOWEVENT:
	      if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
		done = true;
	      break;
	    case SDL_KEYDOWN:
	      switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
		  done = true;
		  break;
		case SDLK_SPACE:
		  process_frames = !process_frames;
		  break;
		}
	      break;
	    }
	}

      static bool is_analmoaly = false;

      double current_second;

      if (process_frames) {
	auto frame = read_frame(hui);
	current_second = frame.timestamp;

	if (frame.pData) {
	  if (frame.type == 'I') {
	    I_packets.add(frame.timestamp, frame.feature1);
	    smooth_i1.add(frame.feature1);
	    smooth_i2.add(frame.feature1);
	    double avg = 0.0;
	    int n = smooth_i1.values.size();
	    int count = std::min((int)smooth_i1.values.size(), 3);
	    for (int i = 0; i < count; i++) {
	      avg += abs(smooth_i1.values[n-i] - smooth_i2.values[n-i]);
	    }
	    if (avg > 1000) {
	      max_i_diff.push_back(max_i_diff.back());
	    } else {
	      max_i_diff.push_back(avg / count);
	    }

	    if (!max_i_diff.empty() && max_i_diff.back() > I_threshold) {
	      add_anomaly(anals, frame.timestamp);
	      is_analmoaly = true;
	    }

	  } else {
	    P_packets.add(frame.timestamp, frame.feature1);
	    smooth_p1.add(frame.feature1);
	    smooth_p2.add(frame.feature1);
	    double avg = 0.0;
	    int n = smooth_p1.values.size();
	    int count = std::min((int)smooth_p1.values.size(), 15);
	    for (int i = 0; i < count; i++) {
	      avg += abs(smooth_p1.values[n-i] - smooth_p2.values[n-i]);
	    }
	    if (avg > 1000) {
	      max_p_diff.push_back(max_p_diff.back());
	    } else {
	      max_p_diff.push_back(avg / count);
	    }

	    if (!max_p_diff.empty() && max_p_diff.back() > P_threshold) {
	      add_anomaly(anals, frame.timestamp);
	      is_analmoaly = true;
	    }

	  }
	} else {
	  process_frames = false;
	  video_done = true;
	}
      }

      if (is_analmoaly && anals.back().end_second + 1.0 < current_second) {
	is_analmoaly = false;
      }

      ImGui_ImplSDLRenderer2_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();

      if (show_demo_window) {
	ImGui::ShowDemoWindow(&show_demo_window);
	ImPlot::ShowDemoWindow();
      }

      ImGui::Begin("Harakiri: timeline");
      if (!video_done) {
	ImGui::Checkbox("Processing frames enabled", &process_frames);
      } else {
	ImGui::TextColored(ImVec4(0.1, 1.0, 0.1, 1.0), "Video is done");
      }
      static char csv_path[512];
      ImGui::InputTextWithHint("CSV path", "write CSV path you donut", csv_path, sizeof(csv_path));
      if (ImGui::Button("Save to csv")) {
	int len = strlen(csv_path);
	sprintf(csv_path+len, "1");
	P_packets.save_csv(csv_path, "times", "P");
	sprintf(csv_path+len, "2");
	I_packets.save_csv(csv_path, "times", "I");
      }
      if (ImPlot::BeginPlot("tyagi tyagi tyagi kefteme")) {
	if (process_frames) {
	  ImPlot::SetupAxes("time","frame size", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
	} else {
	  ImPlot::SetupAxes("time","frame size");
	}
	P_packets.plot();
	I_packets.plot();
	// note: last smooth_p doesn't get drawn
	ImPlot::PlotLine("smooth P1", P_packets.timestamps.data(), smooth_p1.values.data(), P_packets.timestamps.size());
	ImPlot::PlotLine("smooth P2", P_packets.timestamps.data(), smooth_p2.values.data(), P_packets.timestamps.size());
	ImPlot::PlotLine("smooth I1", I_packets.timestamps.data(), smooth_i1.values.data(), I_packets.timestamps.size());
	ImPlot::PlotLine("smooth I2", I_packets.timestamps.data(), smooth_i2.values.data(), I_packets.timestamps.size());

	ImPlot::EndPlot();
      }
      if (ImPlot::BeginPlot("mean diff")) {
	if (process_frames) {
	  ImPlot::SetupAxes("time","frame size", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
	} else {
	  ImPlot::SetupAxes("time","frame size");
	}
	ImPlot::PlotLine("P diff", P_packets.timestamps.data(), max_p_diff.data(), P_packets.timestamps.size());
	ImPlot::PlotLine("I diff", I_packets.timestamps.data(), max_i_diff.data(), I_packets.timestamps.size());
	ImPlot::EndPlot();
      }
      ImGui::End();

      ImGui::Begin("Harakiri: anomaly list");
      static char anal_path[512];
      ImGui::InputTextWithHint("Anomaly path", "write txt path you donut", anal_path, sizeof(anal_path));
      if (ImGui::Button("Save anomalies to file")) {
	FILE* file = fopen(anal_path, "w");
	if (file == NULL) {
	  printf("couldn't open file %s\n!!", anal_path);
	} else {
	  fprintf(file, "{[\n");
	  for (int i = 0; i< anals.size(); i++) {
	    fprintf(file, "{ \"start\" = %.3f, \"end\" = %.3f }%c\n", anals[i].start_second, anals[i].end_second, ", "[i==anals.size()-1]);
	  }
	  fprintf(file, "]}\n");
	  fclose(file);
	}
      }
      for (auto& anal: anals) {
	int b = anal.start_second;
	int e = anal.end_second;
	ImGui::TextColored(ImVec4(0.9, 0.4, 0.3, 1.0), "Anomaly: #start %d:%d #end %d:%d", b/60, b%60, e/60, e%60 );
      }
      ImGui::End();

      ImGui::Render();
      SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
      if (is_analmoaly) {
	SDL_SetRenderDrawColor(renderer, (Uint8)(clear_anal_color.x * 255), (Uint8)(clear_anal_color.y * 255), (Uint8)(clear_anal_color.z * 255), (Uint8)(clear_anal_color.w * 255));
      } else {
	SDL_SetRenderDrawColor(renderer, (Uint8)(clear_fine_color.x * 255), (Uint8)(clear_fine_color.y * 255), (Uint8)(clear_fine_color.z * 255), (Uint8)(clear_fine_color.w * 255));
      }
      SDL_RenderClear(renderer);
      ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
      SDL_RenderPresent(renderer);
    }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  ImPlot::DestroyContext();
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_Quit();
  return 0;
}
