// Copyright (c) 2017-2025 Elderfire Privacy Council
// Copyright (c) 2018-2019 Conceal Network & Conceal Devs
// Copyright (c) 2016-2019 The Karbowanec developers
// Copyright (c) 2012-2018 The CryptoNote developers
//
// This file is part of Fuego.
//
// Fuego is free & open source software distributed in the hope
// that it will be useful, but WITHOUT ANY WARRANTY; without even
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. You may redistribute it and/or modify it under the terms
// of the GNU General Public License v3 or later versions as published
// by the Free Software Foundation. Fuego includes elements written
// by third parties. See file labeled LICENSE for more details.
// You should have received a copy of the GNU General Public License
// along with Fuego. If not, see <https://www.gnu.org/licenses/>

#pragma once

#include <string>

#include <boost/program_options.hpp>

namespace CryptoNote {

class CoreConfig {
public:
  CoreConfig();

  static void initOptions(boost::program_options::options_description& desc);
  void init(const boost::program_options::variables_map& options);

  std::string configFolder;
  bool configFolderDefaulted = true;
};

} //namespace CryptoNote
