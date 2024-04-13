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

  SDL_Window* window = SDL_CreateWindow("how many bananas can fit?",
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

  ImPlot::CreateContext();

  // темная тема
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);

  Ebany_Time_series P_packets = {"P-packet size"};
  Ebany_Time_series I_packets = {"I-packet size"};

  Ebany_Time_series smooth = {"smoothened"};
  float alpha = 0.8;

  bool show_demo_window = true;
  bool process_frames = true;
  bool video_done = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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

      char hex_buff[2048];
      if (process_frames) {
	auto frame = read_frame(hui);
	if (frame.pData) {
	  if (frame.type == 'I') {
	    I_packets.add(frame.timestamp, frame.feature1);
	  } else {
	    P_packets.add(frame.timestamp, frame.feature1);
	  }
	  uint8_t* bytes = (uint8_t*)frame.pData;
	  int offset = 0;
	  for (int i = 0; i < 128; i++) {
	    offset += sprintf(hex_buff+offset, "%x ", bytes[i]);
	  }
	} else {
	  process_frames = false;
	  video_done = true;
	}
      }

      ImGui_ImplSDLRenderer2_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();

      if (show_demo_window) {
	ImGui::ShowDemoWindow(&show_demo_window);
	ImPlot::ShowDemoWindow();
      }

      ImGui::Begin("oh sexy boy");
      if (!video_done) {
	ImGui::Checkbox("Processing frames enabled", &process_frames);
      } else {
	ImGui::TextColored(ImVec4(0.1, 1.0, 0.1, 1.0), "Video is done");
      }
      char csv_path[512];
      ImGui::InputTextWithHint("CSV path", "write CSV path you donut", csv_path, sizeof(csv_path));
      ImGui::TextWrapped("%s", hex_buff);
      if (ImGui::Button("Save to csv")) {
	// TODO
	FILE* file = fopen(csv_path, "w");
	if (!file) {
	  printf("Failed to open file %s\n", csv_path);
	} else {
	  fprintf(file, "time,y\n");
	  for (int i = 0; i < P_packets.timestamps.size(); i++) {
	    fprintf(file, "%.3f,%.3f\n", P_packets.timestamps[i], P_packets.values[i]);
	  }
	  fclose(file);
	  printf("Saving to csv... %s\n", csv_path);
	}
      }
      // ImPlot::SetNextAxisLimits(ImAxis_X1, 0.0, timestamps.back());
      // ImPlot::SetNextAxisLimits(ImAxis_Y1, 0.0, 1.0);
      if (ImPlot::BeginPlot("tyagi tyagi tyagi kefteme")) {
	ImPlot::SetupAxes("time","frame size",ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
	P_packets.plot();
	I_packets.plot();
	ImPlot::EndPlot();
      }
      ImGui::End();

      ImGui::Render();
      SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
      SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
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
