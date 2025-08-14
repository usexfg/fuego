// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free software distributed in the hope that it
// will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You can redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>.

#include "LoggerManager.h"
#include <thread>
#include "ConsoleLogger.h"
#include "FileLogger.h"

namespace Logging {

using Common::JsonValue;

LoggerManager::LoggerManager() {
}

void LoggerManager::operator()(const std::string& category, Level level, boost::posix_time::ptime time, const std::string& body) {
  std::unique_lock<std::mutex> lock(reconfigureLock);
  LoggerGroup::operator()(category, level, time, body);
}

void LoggerManager::configure(const JsonValue& val) {
  std::unique_lock<std::mutex> lock(reconfigureLock);
  loggers.clear();
  LoggerGroup::loggers.clear();
  Level globalLevel;
  if (val.contains("globalLevel")) {
    auto levelVal = val("globalLevel");
    if (levelVal.isInteger()) {
      globalLevel = static_cast<Level>(levelVal.getInteger());
    } else {
      throw std::runtime_error("parameter globalLevel has wrong type");
    }
  } else {
    globalLevel = TRACE;
  }
  std::vector<std::string> globalDisabledCategories;

  if (val.contains("globalDisabledCategories")) {
    auto globalDisabledCategoriesList = val("globalDisabledCategories");
    if (globalDisabledCategoriesList.isArray()) {
      size_t countOfCategories = globalDisabledCategoriesList.size();
      for (size_t i = 0; i < countOfCategories; ++i) {
        auto categoryVal = globalDisabledCategoriesList[i];
        if (categoryVal.isString()) {
          globalDisabledCategories.push_back(categoryVal.getString());
        }
      }
    } else {
      throw std::runtime_error("parameter globalDisabledCategories has wrong type");
    }
  }

  if (val.contains("loggers")) {
    auto loggersList = val("loggers");
    if (loggersList.isArray()) {
      size_t countOfLoggers = loggersList.size();
      for (size_t i = 0; i < countOfLoggers; ++i) {
        auto loggerConfiguration = loggersList[i];
        if (!loggerConfiguration.isObject()) {
          throw std::runtime_error("loggers element must be objects");
        }

        Level level = INFO;
        if (loggerConfiguration.contains("level")) {
          level = static_cast<Level>(loggerConfiguration("level").getInteger());
        }

        std::string type = loggerConfiguration("type").getString();
        std::unique_ptr<Logging::CommonLogger> logger;

        if (type == "console") {
          logger.reset(new ConsoleLogger(level));
        } else if (type == "file") {
          std::string filename = loggerConfiguration("filename").getString();
          auto fileLogger = new FileLogger(level);
          fileLogger->init(filename);
          logger.reset(fileLogger);
        } else {
          throw std::runtime_error("Unknown logger type: " + type);
        }

        if (loggerConfiguration.contains("pattern")) {
          logger->setPattern(loggerConfiguration("pattern").getString());
        }

        std::vector<std::string> disabledCategories;
        if (loggerConfiguration.contains("disabledCategories")) {
          auto disabledCategoriesVal = loggerConfiguration("disabledCategories");
          size_t countOfCategories = disabledCategoriesVal.size();
          for (size_t i = 0; i < countOfCategories; ++i) {
            auto categoryVal = disabledCategoriesVal[i];
            if (categoryVal.isString()) {
              logger->disableCategory(categoryVal.getString());
            }
          }
        }

        loggers.emplace_back(std::move(logger));
        addLogger(*loggers.back());
      }
    } else {
      throw std::runtime_error("loggers parameter has wrong type");
    }
  } else {
    throw std::runtime_error("loggers parameter missing");
  }
  setMaxLevel(globalLevel);
  for (const auto& category : globalDisabledCategories) {
    disableCategory(category);
  }
}

}
