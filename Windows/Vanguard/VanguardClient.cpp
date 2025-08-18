#include "VanguardHelpers.h"

bool VanguardClient::loading = false;
bool VanguardClient::ok_to_corestep = true;
bool VanguardClient::pauseUntilCorrupt = false;
std::string VanguardClient::system_core = "EMPTY";
