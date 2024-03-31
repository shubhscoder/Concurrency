#pragma once

#include <string>
#include <vector>

class Menu { 
    public:
    static Menu create(std::string id) {
        return Menu(id);
    }

    void show() const {}
    void select(int) const {}

    Menu& withTitle(std::string title) {
        title_ = title;
        return *this;
    }

    Menu& horizontal() {
        horizontal_ = true;
        return *this;
    }

    Menu& vertical() {
        horizontal_ = false;
        return *this;
    }

    Menu& addOption(std::string opt) {
        options_.push_back(opt);
        return *this;
    }

    private:

    Menu(std::string id) : id_(id), horizontal_(false) {};

    std::string id_;
    std::string title_;
    bool horizontal_;
    std::vector<std::string> options_;
};

int main() {
    const auto mainMenu = Menu::create("main").withTitle("Dummy").addOption("option1").addOption("option2").horizontal();
    mainMenu.show();
    mainMenu.select(1);
}