#include "command/Command.h"
#include "github/webhook/WebhookCommand.h"
#include "pdb/PdbCommand.h"
#include <cstring>
#include <iostream>
#include <pcre2.h>

#ifdef _WIN32
#include <Windows.h>
#endif

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    (void)argc;
    (void)argv;
    github::webhook::WebhookCommand::init();
    pdb::PdbCommand::init();
    while (command::Command::instance()->execute()) {
    }
    return 0;
}
