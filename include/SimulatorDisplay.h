// #pragma once
// #include <SDL2/SDL.h>
//
// #include <iostream>
//
// class SimulatorDisplay {
//  public:
//   SDL_Window* window = nullptr;
//   SDL_Renderer* renderer = nullptr;
//
//   void begin() {
//     SDL_Init(SDL_INIT_VIDEO);
//     window = SDL_CreateWindow("CrossInk Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 800, 0);
//     renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
//     // Set background to e-ink white
//     SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
//     SDL_RenderClear(renderer);
//     SDL_RenderPresent(renderer);
//   }
//
//   // Mocking the E-Ink drawing function
//   void drawPixel(int x, int y, uint16_t color) {
//     if (color == 0)
//       SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black
//     else
//       SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // White
//
//     SDL_RenderDrawPoint(renderer, x, y);
//   }
//
//   void display() { SDL_RenderPresent(renderer); }
//
//   void handleEvents() {
//     SDL_Event e;
//     while (SDL_PollEvent(&e)) {
//       if (e.type == SDL_QUIT) exit(0);
//     }
//   }
// };

#pragma once
#include <SDL2/SDL.h>
#include <iostream>

class SimulatorDisplay {
public:
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  bool quit_requested = false; // Thêm cờ để báo hiệu thoát an toàn thay vì crash/exit đột ngột

  void begin() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
      return;
    }

    // Tạo cửa sổ mô phỏng màn hình E-Ink 480x800 dọc
    window = SDL_CreateWindow("CrossInk Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480, 800, 0);
    if (!window) {
      std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
      return;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Khởi tạo nền màu trắng đục đặc trưng của giấy điện tử (E-ink White)
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }

  // Giả lập hàm vẽ Pixel của màn hình nhúng
  void drawPixel(int x, int y, uint16_t color) {
    // Tránh việc đổi màu draw color không cần thiết nếu tọa độ nằm ngoài màn hình
    if (x < 0 || x >= 480 || y < 0 || y >= 800) return;

    if (color == 0)
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);       // Đen (Mực e-ink)
      else
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Trắng

        SDL_RenderDrawPoint(renderer, x, y);
  }

  // Đẩy toàn bộ dữ liệu đã vẽ lên màn hình máy tính
  void display() {
    SDL_RenderPresent(renderer);
  }

  // Xử lý các sự kiện chuột/bàn phím của cửa sổ máy tính
  void handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit_requested = true;
      }
    }
  }

  // Hàm dọn dẹp bộ nhớ khi tắt Simulator (Rất quan trọng để tránh rò rỉ bộ nhớ máy tính)
  void end() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
  }
};
