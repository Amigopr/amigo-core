 /*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, token to the following conditions:
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
#include <graphene/chain/operation_wrapper_evaluator.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/db/object_database.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/variant.hpp>
#include <fc/time.hpp>
#include <ctype.h>


namespace graphene { namespace chain {



void_result operation_wrapper_evaluator::do_evaluate( const operation_wrapper_operation& op )
{ try {
	if(op.operation_name=="")
	{
		elog("operation_wrapper_evaluator::do_evaluate() operation_name is NULL");
	}
	else
	{
		elog("operation_wrapper_evaluator::do_evaluate() unknown operation_name(${a})", ("a", op.operation_name));
	}
	return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void operation_wrapper_evaluator::pay_fee()
{
	// input to fee pool
	deferred_fee_ = core_fee_paid;
	ilog("operation_wrapper, fee=${fee}", ("fee", deferred_fee_));

}


void_result operation_wrapper_evaluator::do_apply( const operation_wrapper_operation& op )
{ try {
	if(op.operation_name=="")
	{
		elog("operation_wrapper_evaluator::do_evaluate() operation_name is NULL");
	}
	else
	{
		elog("operation_wrapper_evaluator::do_evaluate() unknown operation_name(${a})", ("a", op.operation_name));
	}
	return void_result();

} FC_CAPTURE_AND_RETHROW( (op) ) }

} } // graphene::chain


