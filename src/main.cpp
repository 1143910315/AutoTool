#include "command/Command.h"
#include "github/webhook/WebhookCommand.h"
#include "ll/HookAllCommand.h"
#include "pdb/PdbCommand.h"

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
    ll::HookAllCommand::init();
    while (command::Command::instance()->execute()) {
    }
    return 0;
}
