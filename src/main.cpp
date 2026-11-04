#include "include/main_base.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    PetWidget pet;
    pet.show();
    return app.exec();  // 进入事件循环
}