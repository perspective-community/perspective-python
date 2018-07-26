/******************************************************************************
 *
 * Copyright (c) 2017, the Perspective Authors.
 *
 * This file is part of the Perspective library, distributed under the terms of
 * the Apache License 2.0.  The full license can be found in the LICENSE file.
 *
 */

#include <perspective/first.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <perspective/raw_types.h>
#include <string>
#include <sstream>


namespace perspective
{

t_str
unique_path(const t_str& path_prefix)
{
	std::stringstream ss;
	ss << path_prefix << boost::uuids::random_generator()();
	return ss.str();
}
} // end
