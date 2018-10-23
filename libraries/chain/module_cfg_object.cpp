/*
 * Copyright (c) 2017 Amigo, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/module_cfg_object.hpp>
#include <graphene/chain/database.hpp>

#include <fc/uint128.hpp>

#include <cmath>

using namespace graphene::chain;
using namespace fc;


module_cfg_object::module_cfg_object(const module_cfg_object& cfg_obj)
{
	id = cfg_obj.id;
	module_name = cfg_obj.module_name;
	last_update_time = cfg_obj.last_update_time;
	last_modifier = cfg_obj.last_modifier;
	module_cfg = mutable_variant_object(cfg_obj.module_cfg);
	pending_module_cfg = mutable_variant_object(cfg_obj.pending_module_cfg);
}

module_cfg_object& module_cfg_object::operator=(const module_cfg_object& cfg_obj)
{
	if (this == &cfg_obj)
		return *this;
	id = cfg_obj.id;
	module_name = cfg_obj.module_name;
	last_update_time = cfg_obj.last_update_time;
	last_modifier = cfg_obj.last_modifier;
	module_cfg = mutable_variant_object(cfg_obj.module_cfg);
	pending_module_cfg = mutable_variant_object(cfg_obj.pending_module_cfg);
	return *this;
}

