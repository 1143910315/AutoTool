#include "CommandCallback.h"

command::CommandCallback::CommandCallback(std::function<bool()> callback) : callbackFunction(callback) {
}

bool command::CommandCallback::execute() {
    if (callbackFunction()) {
        return command::Command::execute();
    }
    return true;
}
