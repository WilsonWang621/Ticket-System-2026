#include <../include/ticket_system.h>
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    sjtu::TicketSystem system;
    system.run();

    return 0;
}
