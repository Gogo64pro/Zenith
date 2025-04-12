//
// Created by gogop on 3/30/2025.
//

#pragma once

#include <string>
#include <stdexcept>
#include <fstream>

namespace zenith::utils {

std::string readFile(std::string_view filePath);

} // zenith::utils