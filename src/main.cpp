#include <array>
#include <functional>
#include <iostream>
#include <random>

#include <docopt/docopt.h>
#include <ftxui/component/captured_mouse.hpp> // for ftxui
#include <ftxui/component/component.hpp> // for Slider
#include <ftxui/component/component_options.hpp> // for ButtonOption
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/screen/color.hpp> // for Color, Color::Blue, Color::Green, Color::Red
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/flexbox_config.hpp>

#include <spdlog/spdlog.h>

// This file will be generated automatically when you run the CMake
// configuration step. It creates a namespace called `ftxui_test`. You can modify
// the source template at `configured_files/config.hpp.in`.
#include <internal_use_only/config.hpp>

template<std::size_t Width, std::size_t Height> 
struct GameBoard
{
  static constexpr std::size_t width = Width;
  static constexpr std::size_t height = Height;

  std::array<std::array<std::string, height>, width> strings;
  std::array<std::array<bool, height>, width> values{};

  std::size_t move_count{ 0 };

  std::string &get_string(std::size_t x, std::size_t y) { return strings.at(x).at(y); }


  void set(std::size_t x, std::size_t y, bool new_value)
  {
    get(x, y) = new_value;

    if (new_value) {
      get_string(x, y) = "-*-";
    } else {
      get_string(x, y) = "   ";
    }
  }

  void visit(auto visitor)
  {
    for (std::size_t x = 0; x < width; ++x) {
      for (std::size_t y = 0; y < height; ++y) { visitor(x, y, *this); }
    }
  }

  [[nodiscard]] bool get(std::size_t x, std::size_t y) const { return values.at(x).at(y); }

  [[nodiscard]] bool &get(std::size_t x, std::size_t y) { return values.at(x).at(y); }

  GameBoard()
  {
    visit([](const auto x, const auto y, auto &gameboard) { gameboard.set(x, y, true); });
  }

  void update_strings()
  {
    for (std::size_t x = 0; x < width; ++x) {
      for (std::size_t y = 0; y < height; ++y) { set(x, y, get(x, y)); }
    }
  }

  void toggle(std::size_t x, std::size_t y) { set(x, y, !get(x, y)); }

  void press(std::size_t x, std::size_t y)
  {
    ++move_count;
    toggle(x, y);
    if (x > 0) { toggle(x - 1, y); }
    if (y > 0) { toggle(x, y - 1); }
    if (x < width - 1) { toggle(x + 1, y); }
    if (y < height - 1) { toggle(x, y + 1); }
  }

  [[nodiscard]] bool solved() const
  {
    for (std::size_t x = 0; x < width; ++x) {
      for (std::size_t y = 0; y < height; ++y) {
        if (!get(x, y)) { return false; }
      }
    }

    return true;
  }
};


void consequence_game()
{
  auto screen = ftxui::ScreenInteractive::TerminalOutput();

  GameBoard<3, 3> gb;

  std::string moves_text;

  const auto update_moves = [&moves_text](const auto &game_board) {
    moves_text = fmt::format("{}", game_board.move_count);
    if (game_board.solved()) { moves_text += " Solved!"; }
  };

  const auto make_buttons = [&] {
    std::vector<ftxui::Component> buttons;

    for (std::size_t x = 0; x < gb.width; ++x) {
      for (std::size_t y = 0; y < gb.height; ++y) {
        buttons.push_back(ftxui::Button(&gb.get_string(x, y), [=, &gb] {
          if (!gb.solved()) { gb.press(x, y); }
          update_moves(gb);
        }));
      }
    }
    return buttons;
  };


  auto buttons = make_buttons();

  const auto reset_board = [&] {
    
    static constexpr int randomization_iterations = 100;
    static std::random_device rd;
    static std::mt19937 gen32{ rd() };
    static std::uniform_int_distribution<std::size_t> x(static_cast<std::size_t>(0), gb.width - 1);
    static std::uniform_int_distribution<std::size_t> y(static_cast<std::size_t>(0), gb.height - 1);

    do {
      for (int i = 0; i < randomization_iterations; ++i) { 
        gb.press(x(gen32), y(gen32)); 
      }  
    } while(gb.solved());
    
    gb.move_count = 0;
    update_moves(gb);

  };
  
  auto reset_button = ftxui::Button("Reset", reset_board); 
  auto quit_button = ftxui::Button("Quit", screen.ExitLoopClosure());

  auto set_size = [](ftxui::Element elem, int dimx, int dimy)
  {
    return elem | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, dimx) | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, dimy);
  };

  auto make_box = [&set_size](std::string title, std::string content, int dimx, int dimy) {
    return set_size(
      ftxui::window(
        ftxui::text(title) | ftxui::hcenter,
        ftxui::text(content) | ftxui::hcenter | ftxui::dim
      ),
      dimx,
      dimy
    );
  };

  auto make_layout = [&] {
    std::vector<ftxui::Element> rows;

    std::size_t idx = 0;

    rows.push_back(ftxui::hbox({ make_box("", "Lights Out", 15, 3) }));

    for (std::size_t x = 0; x < gb.width; ++x) {
      std::vector<ftxui::Element> row;
      for (std::size_t y = 0; y < gb.height; ++y) {
        row.push_back(buttons[idx]->Render());
        ++idx;
      }
      rows.push_back(ftxui::hbox(std::move(row)));
    }

    rows.push_back(ftxui::hbox({ make_box("Moves", moves_text, 15, 3) }));
    
    ftxui::FlexboxConfig config;
    config.direction = ftxui::FlexboxConfig::Direction::Row;
    config.wrap = ftxui::FlexboxConfig::Wrap::Wrap;
    config.justify_content = ftxui::FlexboxConfig::JustifyContent::SpaceBetween;
    config.align_items = ftxui::FlexboxConfig::AlignItems::FlexStart;
    config.align_content = ftxui::FlexboxConfig::AlignContent::FlexStart;

    auto button_group = ftxui::flexbox({ reset_button->Render(), quit_button->Render() }, config);
    button_group = button_group | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 15);
    button_group = hbox(button_group, ftxui::filler());
    
    rows.push_back(button_group);
    
    return ftxui::vbox(std::move(rows));
  };

  reset_board();

  auto all_buttons = buttons;
  all_buttons.push_back(reset_button);
  all_buttons.push_back(quit_button);

  auto container = ftxui::Container::Horizontal(all_buttons);

  auto renderer = ftxui::Renderer(container, make_layout);

  screen.Loop(renderer);
}

int main(int argc, const char **argv)
{
  try {
    static constexpr auto USAGE =
      R"(intro

    Usage:
          intro
          intro (-h | --help)
          intro --version
 Options:
          -h --help     Show this screen.
          --version     Show version.
)";

    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
      { std::next(argv), std::next(argv, argc) },
      true,// show help if requested
      fmt::format("{} {}",
        ftxui_test::cmake::project_name,
        ftxui_test::cmake::project_version));// version string, acquired
                                            // from config.hpp via CMake

    //if (args["turn_based"].asBool()) {
      consequence_game();
    //} else {
    //  game_iteration_canvas();
    //}

    //    consequence_game();
  } catch (const std::exception &e) {
    fmt::print("Unhandled exception in main: {}", e.what());
  }
}
