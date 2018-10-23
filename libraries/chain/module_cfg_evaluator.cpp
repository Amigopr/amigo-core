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
#include <graphene/chain/module_cfg_evaluator.hpp>
#include <graphene/chain/module_cfg_object.hpp>
#include <graphene/chain/module_configurator.hpp>
#include <graphene/chain/exceptions.hpp>
#include <fc/io/json.hpp>
#include <fc/variant_object.hpp>

#include <functional>


namespace graphene { namespace chain {

static const string DELETE_OP_PHRASE = "delete";

void_result module_cfg_evaluator::evaluate_cfg(
	const variant_object& current_cfg, 
	const variant_object& new_cfg,
	uint8_t op_type
	)
{
  ilog("current_cfg: ${current_cfg}, new_cfg: ${new_cfg}", ("current_cfg", current_cfg)("new_cfg", new_cfg));
   for (auto it_new = new_cfg.begin(); it_new != new_cfg.end(); ++it_new)
   {
   	    const string& cfg_key = it_new->key();
   	    if (op_type == module_cfg_op_insert)
   	    {
          //对于插入操作，检查要插入的叶子节点是否已存在
          if (!current_cfg.contains(cfg_key.c_str()))
          {
            //不包含cfg_key，是新插入的，跳过进一步检查
            continue;
          }
   	    	else
          {
            //cfg_key已存在，则必须是非叶子节点（即类型应为object），不能重复插入叶子节点
            FC_ASSERT(current_cfg[cfg_key].is_object(), "config parameter has already exists: ${cfg_key}", ("cfg_key", cfg_key));
            //输入参数的类型也要是object（键值对类型）
            FC_ASSERT(new_cfg[cfg_key].is_object(), "input parameter type is not object: ${cfg_key}", ("cfg_key", cfg_key));
          }
   	    }
   	    else
   	    {
   	    	FC_ASSERT(current_cfg.contains(cfg_key.c_str()), "config parameter not supported: ${cfg_key}", ("cfg_key", cfg_key));
   	    }
   	    
   	    //对于delete操作，如果new_cfg[cfg_key]值不为delete，说明要删的是子层级参数。当前层级参数类型依然要检查
   	    if (op_type == module_cfg_op_update || !(new_cfg[cfg_key].is_string() && (new_cfg[cfg_key].as_string() == DELETE_OP_PHRASE)))
   	    {
   	    	FC_ASSERT(new_cfg[cfg_key].get_type() == current_cfg[cfg_key].get_type(), "parameter type mismatched for ${cfg_key}, required: ${required}, passed in: ${passed}",
   	    	    ("cfg_key", cfg_key)("required",current_cfg[cfg_key].get_type())("passed", new_cfg[cfg_key].get_type()));
   	    }

   	    if (current_cfg[cfg_key].is_object())
   	    {
   	    	//对于不删的key才进一步校验
   	    	if (!(new_cfg[cfg_key].is_string() && (new_cfg[cfg_key].as_string() == DELETE_OP_PHRASE)))
   	    	    return evaluate_cfg(current_cfg[cfg_key].get_object(), new_cfg[cfg_key].get_object(), op_type);
   	    }
   }
   return void_result();
}

void_result module_cfg_evaluator::do_evaluate(const module_cfg_operation& o)
{ try {
   ilog("evaluate op: ${op}", ("op", o));
   string cfg_str = fc::json::to_string(o.cfg_value);
   ilog("cfg_str: ${cfg}", ("cfg", cfg_str));
   database& d = db();

   //auth verify
   FC_ASSERT(trx_state->_is_proposed_trx, "not a proposal transaction");
   //TODO 创建一批多重签名账号？

   //common evaluation
   const auto& cfg_objs = d.get_index_type<module_cfg_index>().indices().get<by_name>();
   auto itr = cfg_objs.find(o.module_name);
   if (o.op_type == module_cfg_op_insert)
   {
        if (itr != cfg_objs.end())
        {
            //插入的时候，不要求cfg object一定已存在（没有的话会创建），但是如果已存在的话，校验下要配置的参数，防止错误插入一个已存在的参数
       	    const variant_object& current_cfg = itr->module_cfg;
            const variant_object& new_cfg = o.cfg_value;
            evaluate_cfg(current_cfg, new_cfg, o.op_type);
        }
    }
    else
    {
   	    FC_ASSERT(itr !=cfg_objs.end(), "module configuration object not found: ${name}", ("name", o.module_name));
        const variant_object& current_cfg = itr->module_cfg;
        const variant_object& new_cfg = o.cfg_value;
        evaluate_cfg(current_cfg, new_cfg, o.op_type);
    }

	auto& configurator_prt = d.get_configurator(o.module_name);
	FC_ASSERT(configurator_prt != 0, "configurator not found for module ${name}", ("name", o.module_name));
	//module specific evaluation
	configurator_prt->set_database(&d);
	return configurator_prt->evaluate(o);
} FC_CAPTURE_AND_RETHROW((o)) }

void_result module_cfg_evaluator::apply_cfg(
	fc::mutable_variant_object& current_cfg, 
	const variant_object& new_cfg,
	uint8_t op_type
	)
{
	//database& d = db();
	//evaluate过了，这里不再做检查
    for (auto it_new = new_cfg.begin(); it_new != new_cfg.end(); ++it_new)
    {
   	    const string& cfg_key = it_new->key();
   	    if (current_cfg.find(cfg_key) == current_cfg.end())
   	    {
   	    	//new param, insert it
   	    	current_cfg[cfg_key] = new_cfg[cfg_key];
   	    	continue;
   	    }
   	    if (current_cfg[cfg_key].is_object() && new_cfg[cfg_key].is_object())
   	    {
   	    	fc::mutable_variant_object mvo(current_cfg[cfg_key].get_object());
   	    	apply_cfg(mvo, new_cfg[cfg_key].get_object(), op_type);
   	    	current_cfg[cfg_key] = mvo;
   	    }
   	    else
   	    {
   	    	//非object类型，直接覆盖或删除
   	    	if (op_type == module_cfg_op_update)
   	    	{
   	    		current_cfg[cfg_key] = new_cfg[cfg_key];
   	    	}
   	    	else
   	    	{
   	    		//delete param
   	    		current_cfg.erase(cfg_key);
   	    	}
   	    }
    }
    return void_result();
}

void_result module_cfg_evaluator::do_apply(const module_cfg_operation& o)
{ try {

	database& d = db();
    const auto& cfg_objs = d.get_index_type<module_cfg_index>().indices().get<by_name>();
    auto itr = cfg_objs.find(o.module_name);
    if (itr == cfg_objs.end())
    {
    	//first time to insert a param, create cfg object first
    	const module_cfg_object& cfg_obj = d.create<module_cfg_object>( [&]( module_cfg_object& cfg_obj ) {
		    cfg_obj.module_name = o.module_name;
		    cfg_obj.module_cfg = o.cfg_value;
	        cfg_obj.last_update_time = d.head_block_time();
	        cfg_obj.last_modifier = o.proposer;
		  });
		  ilog("config object created for module ${name}. id: ${id}", ("name", o.module_name)("id", cfg_obj.id));
    }
    else
    {
    	// update or delete
    	const variant_object& current_cfg = itr->module_cfg;
        fc::mutable_variant_object current_cfg_tmp(current_cfg);
        const variant_object& new_cfg = o.cfg_value;
        //common apply, settup new cfg
        //modify a copy first
        apply_cfg(current_cfg_tmp, new_cfg, o.op_type);
        //save copy to db
   	    d.modify( *itr, [&]( module_cfg_object& cfg_obj ){
	        cfg_obj.module_cfg = current_cfg_tmp;
	        cfg_obj.last_update_time = d.head_block_time();
	        cfg_obj.last_modifier = o.proposer;
	    });
    }

    //TODO 这里通知相关模块，配置有更新。目前限定配置器存在的时候通知。如果要解耦，得用更复杂的方式
	auto& configurator_prt = d.get_configurator(o.module_name);
	configurator_prt->set_database(&d);
	//module specific config, notify module the change of configuration
	return configurator_prt->apply(o);
} FC_CAPTURE_AND_RETHROW((o)) }

} } // graphene::chain
