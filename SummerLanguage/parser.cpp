#include "parser.h"
#include <cassert>

namespace summer_lang
{
	parser::parser()
	{
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmParser();
		llvm::InitializeNativeTargetAsmPrinter();
		auto & context = llvm::getGlobalContext();
		global_JIT_helper = std::make_unique<MCJIT_helper>(context);

		global_op_precedence["="] = 2;
		global_op_precedence["<"] = 10;
		global_op_precedence[">"] = 10;
		global_op_precedence[">="] = 10;
		global_op_precedence["<="] = 10;
		global_op_precedence["=="] = 10;
		global_op_precedence["<>"] = 10;

		global_op_precedence["+"] = 20;
		global_op_precedence["-"] = 20;
		global_op_precedence["*"] = 40;
		global_op_precedence["/"] = 40;

		lib::import();
	}

	void parser::handle_extern()
	{
		if (auto proto_ast = parse_extern_())
		{
			if (auto ir = proto_ast->codegen())
			{
				ir->dump();
			}
		}
		else
			get_next_token_();
	}

	void parser::handle_function()
	{
		if (auto func_ast = parse_function_())
		{
			if (auto ir = func_ast->codegen())
			{
				ir->dump();
			}
		}
		else
			get_next_token_();
	}

	void parser::handle_top_level_expr()
	{
		if (auto func_ast = parse_top_level_expr_())
		{
			if (auto ir = func_ast->codegen())
			{
				ir->dump();
				auto p_function = (double(*)())(intptr_t)global_JIT_helper->get_pointer_to_function(ir);
				p_function();
			}
		}
		else
			get_next_token_();
	}

	void parser::parse(const std::string & file_name)
	{
		std::ifstream source_code(file_name.c_str());
		if (!source_code.is_open())
			throw std::fstream::failure("Can't open file " + file_name);
		else
		{
			p_tokenizer_ = std::make_unique<tokenizer>(source_code);

			get_next_token_();
			while (true)
			{
				switch (current_token_->get_type())
				{
				case token_categories::END:
					return;
				case token_categories::KEYWORD:
				{
					auto type = get_value<keyword>(current_token_);
					switch (type)
					{
					case keyword_categories::EXTERN:
						handle_extern();
						continue;
					case keyword_categories::FUNCTION:
						handle_function();
						continue;
					default:
						break;
					}
				}
				default:
					handle_top_level_expr();
					break;
				}
			}
			p_tokenizer_.reset();
		}
	}

	void parser::get_next_token_()
	{
		current_token_ = std::move(p_tokenizer_->get_token());
	}

	int get_op_precedence(const std::string & op)
	{
		if (global_op_precedence.find(op) != global_op_precedence.end())
			return global_op_precedence[op];
		return -1;
	}

	void set_op_precedence(const std::string & op, int precedence)
	{
		global_op_precedence[op] = precedence;
	}

	llvm::AllocaInst * global_create_alloca(llvm::Function * parent, const std::string & name, llvm::Type * type)
	{
		llvm::IRBuilder<> temp_block(&(parent->getEntryBlock()), parent->getEntryBlock().begin());
		return temp_block.CreateAlloca(type, 0, name.c_str());
	}

	std::unique_ptr<ast> parser::parse_number_()
	{
		auto start_row_no = current_token_->get_position();
		auto result = std::make_unique<number_ast>(get_value<literal_number>(current_token_), start_row_no);
		get_next_token_();
		return std::move(result);
	}

	std::unique_ptr<ast> parser::parse_parenthesis_()
	{
		get_next_token_();

		auto result = parse_expression_();
		if (!result)
			return result;

		if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::RBRACKET)
			throw syntax_error("Expected a ')'", current_token_->get_position());
		get_next_token_();
		return result;
	}

	std::unique_ptr<ast> parser::parse_identifier_()
	{
		auto name = get_value<identifier>(current_token_);
		auto start_row_no = current_token_->get_position();

		get_next_token_();
		if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::LBRACKET)
			return std::make_unique<variable_ast>(name, start_row_no);
		
		get_next_token_();
		std::vector<std::unique_ptr<ast>> args;
		if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::RBRACKET)
		{
			while (true)
			{
				auto arg = parse_expression_();
				if (!arg)
					return nullptr;
				args.push_back(std::move(arg));

				if (current_token_->get_type() == token_categories::OPERATOR && get_value<op>(current_token_) == operator_categories::RBRACKET)
					break;

				if (current_token_->get_type() != token_categories::OPERATOR ||  get_value<op>(current_token_) != operator_categories::COMM)
					throw syntax_error("Expect a ')' or ',' in argument list", current_token_->get_position());

				get_next_token_();
			}
		}

		get_next_token_();
		return std::make_unique<call_expression_ast>(name, std::move(args), start_row_no);
	}

	std::unique_ptr<ast> parser::parse_primary_()
	{
		switch (current_token_->get_type())
		{
		case token_categories::LITERAL_NUMBER:
			return parse_number_();
		case token_categories::IDENTIFIER:
			return parse_identifier_();
		case token_categories::KEYWORD:
		{
			auto type = get_value<keyword>(current_token_);
			switch (type)
			{
			case keyword_categories::IF:
				return parse_if_();
			case keyword_categories::FOR:
				return parse_for_();
			case keyword_categories::VAR:
				return parse_var_();
			case keyword_categories::RETURN:
				return parse_return_();
			default:
				break;
			}
			break;
		}
		case token_categories::OPERATOR:
			switch (get_value<op>(current_token_))
			{
			case operator_categories::LBRACKET:
				return parse_parenthesis_();
			case operator_categories::SEMI:
				return parse_empty_();
			default:
				break;
			}
		default:
			break;
		}
		throw syntax_error("Unknown token", current_token_->get_position());
	}

	std::unique_ptr<ast> parser::parse_expression_()
	{
		auto start_row_no = current_token_->get_position();
		auto left = parse_unary_();

		return parse_bin_op_right_(0, std::move(left), start_row_no);
	}

	std::unique_ptr<ast> parser::parse_bin_op_right_(int expr_precedence, std::unique_ptr<ast> left, int start_row_no)
	{
		while (true)
		{ 
			if (current_token_->get_type() == token_categories::OPERATOR)
			{
				auto current_op = get_op_name(current_token_);
				auto current_op_type = get_value<op>(current_token_);
				auto current_precedence = get_op_precedence(current_op);

				if (current_precedence < expr_precedence)
					return left;

				get_next_token_();
				auto right = parse_unary_();
				if (!right)
					return nullptr;

				if (current_token_->get_type() == token_categories::OPERATOR)
				{
					auto next_op = get_op_name(current_token_);
					auto next_precedence = get_op_precedence(next_op);

					if (current_precedence < next_precedence)
					{
						right = parse_bin_op_right_(current_precedence + 1, std::move(right), start_row_no);
						if (!right)
							return nullptr;
					}
				}

				left = std::make_unique<binary_expression_ast>(current_op,  current_op_type, std::move(left), std::move(right), start_row_no);
			}
			else
				return  left;
		}
	}

	std::unique_ptr<ast> parser::parse_if_()
	{
		auto start_row_no = current_token_->get_position();
		get_next_token_();

		auto cond = parse_expression_();
		if (!cond)
			return nullptr;

		if (current_token_->get_type() != token_categories::KEYWORD || get_value<keyword>(current_token_) != keyword_categories::THEN)
			throw syntax_error("Expected \'then\' keyword", current_token_->get_position());
		get_next_token_();

		auto then_part = parse_block_();
		if (!then_part)
			return nullptr;

		if (current_token_->get_type() != token_categories::KEYWORD || get_value<keyword>(current_token_) != keyword_categories::ELSE)
			throw syntax_error("Expected \'else\' keyword", current_token_->get_position());
		get_next_token_();

		auto else_part = parse_block_();
		if (!else_part)
			return nullptr;

		return std::make_unique<if_expression_ast>(std::move(cond), std::move(then_part), std::move(else_part), start_row_no);

	}

	std::unique_ptr<ast> parser::parse_for_()
	{															
		auto start_row_no = current_token_->get_position();
		get_next_token_();

		if (current_token_ -> get_type() != token_categories::IDENTIFIER)
			throw syntax_error("Expected a identifier after \'for\'", current_token_->get_position());
		auto var_name = get_value<identifier>(current_token_);
		get_next_token_();

		if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::COLON)
			throw syntax_error("Expected a type", current_token_->get_position());
		get_next_token_();

		if (current_token_->get_type() != token_categories::TYPE || get_value<type>(current_token_) == type_categories::VOID)
			throw syntax_error("Expected a correct type", current_token_->get_position());
		llvm::Type * var_type;
		switch (get_value<type>(current_token_))
		{
		case type_categories::NUMBER:
			var_type = llvm::Type::getDoubleTy(llvm::getGlobalContext());
			break;
		default:
			throw syntax_error("Unknown type", current_token_->get_position());
		}
		get_next_token_();

		if (current_token_ ->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::ASSIGN)
			throw syntax_error("Expected a '=' after \'for\'", current_token_->get_position());
		get_next_token_();

		auto start = parse_expression_();
		if (!start)
			return nullptr;

		if (current_token_ ->get_type() != token_categories::OPERATOR ||get_value<op>(current_token_) != operator_categories::COMM)
			throw syntax_error("Expected a ',' between parts of \'for\'", current_token_->get_position());
		get_next_token_();
	
		auto end = parse_expression_();
		if (!end)
			return nullptr;

		std::unique_ptr<ast> step;
		if (current_token_->get_type() == token_categories::OPERATOR && get_value<op>(current_token_) == operator_categories::COMM)
		{
			get_next_token_();
			step = parse_expression_();
			if (!step)
				return nullptr;
		}
		if (!step)
			step.reset(new number_ast(1, current_token_->get_position()));

		if (current_token_->get_type() != token_categories::KEYWORD || get_value<keyword>(current_token_) != keyword_categories::IN)
			throw syntax_error("Expected \"in\" between head and body of \'for\'", current_token_->get_position());
		get_next_token_();
		
		auto body = parse_block_();
		if (!body)
			return nullptr;

		return std::make_unique<for_expression_ast>(var_name, var_type, std::move(start), std::move(end), std::move(step), std::move(body), start_row_no);
	}

	std::unique_ptr<ast> parser::parse_unary_()
	{
		if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::USER_DEFINED)
			return parse_primary_();

		auto start_row_no = current_token_->get_position();
		auto op = get_op_name(current_token_);
		get_next_token_();
		if (auto expr = parse_unary_())
			return std::make_unique<unary_expression_ast>(op, std::move(expr), start_row_no);
		return nullptr;
	}

	std::unique_ptr<ast> parser::parse_var_()
	{
		auto start_row_no = current_token_->get_position();
		get_next_token_();

		std::vector<std::tuple<std::string, llvm::Type *, std::unique_ptr<ast>>> vars;
		if (current_token_->get_type() != token_categories::IDENTIFIER)
			throw syntax_error("Expected identifier after \'var\'", current_token_->get_position());

		while (true)
		{
			auto var_name = get_value<identifier>(current_token_);
			get_next_token_();

			if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::COLON)
				throw syntax_error("Expected a \':\' between name and type", current_token_->get_position());
			get_next_token_();

			if (current_token_->get_type() != token_categories::TYPE || get_value<type>(current_token_) == type_categories::VOID)
				throw syntax_error("Expected a legal type name", current_token_->get_position());
			llvm::Type * var_type;
			switch (get_value<type>(current_token_))
			{
			case type_categories::NUMBER:
				var_type = llvm::Type::getDoubleTy(llvm::getGlobalContext());
				break;
			default:
				throw syntax_error("Unknown type", current_token_->get_position());
			}
			get_next_token_();

			if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::ASSIGN)
				throw syntax_error("Expected initialization of variable" + var_name, current_token_->get_position());
			get_next_token_();

			auto init = parse_expression_();
			vars.push_back(std::make_tuple(var_name, var_type, std::move(init)));

			if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::COMM)
				break;
			get_next_token_();
			
			if (current_token_->get_type() != token_categories::IDENTIFIER)
				throw syntax_error("Expected identifier list after \'var\'", current_token_->get_position());
		}

		if (current_token_->get_type() != token_categories::KEYWORD || get_value<keyword>(current_token_) != keyword_categories::IN)
			throw syntax_error("Expected 'in' keyword after \'var\'", current_token_->get_position());

		get_next_token_();
		auto body = parse_block_();

		return std::make_unique<var_ast>(std::move(vars), std::move(body), start_row_no);
	}

	std::unique_ptr<ast> parser::parse_block_()
	{
		auto start_row_no = current_token_->get_position();

		if (current_token_->get_type() != token_categories::KEYWORD || get_value<keyword>(current_token_) != keyword_categories::BEGIN)
			throw syntax_error("Expected a 'begin' keyword at the start of block", current_token_->get_position());
		get_next_token_();

		std::vector<std::unique_ptr<ast>> exprs;
		while (current_token_->get_type() != token_categories::KEYWORD || get_value<keyword>(current_token_) != keyword_categories::END)
		{
			auto expr = parse_expression_();
			exprs.push_back(std::move(expr));
			//if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::SEMI)
			//	throw syntax_error("Expected a \';\' after each expression in block", current_token_->get_position());
			//get_next_token_();
		}

		get_next_token_();
		return std::make_unique<block_ast>(std::move(exprs), start_row_no);
	}

	std::unique_ptr<ast> parser::parse_return_()
	{
		auto start_row_no = current_token_->get_position();
		get_next_token_();

		auto ret = parse_expression_();
		return std::make_unique<return_ast>(std::move(ret), start_row_no);
	}

	std::unique_ptr<ast> parser::parse_empty_()
	{
		auto start_row_no = current_token_->get_position();
		get_next_token_();
		return std::make_unique<empty_ast>(start_row_no);
	}

	std::unique_ptr<prototype_ast> parser::parse_prototype_()
	{
		std::string name;
		auto kind = 0;		//0 = identifier, 1 = unary, 2 = binary
		auto precedence = 20;		//for binary operator
		auto start_row_no = current_token_->get_position();

		switch (current_token_->get_type())
		{
		case token_categories::IDENTIFIER:
			name = get_value<identifier>(current_token_);
			kind = 0;
			get_next_token_();
			break;
		case token_categories::KEYWORD:
			switch (get_value<keyword>(current_token_))
			{
			case keyword_categories::BINARY:
				get_next_token_();
				if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::USER_DEFINED)
					throw syntax_error("Expected a user-defined operator", current_token_->get_position());
				name = "binary" + get_op_name(current_token_);
				kind = 2;
				get_next_token_();

				if (current_token_->get_type() == token_categories::LITERAL_NUMBER)
				{
					precedence = get_value<literal_number>(current_token_);
					get_next_token_();
				}
				break;
			case keyword_categories::UNARY:
				get_next_token_();
				if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::USER_DEFINED)
					throw syntax_error("Expected a user-defined operator", current_token_->get_position());
				name = "unary" + get_op_name(current_token_);
				kind = 1;
				get_next_token_();
				break;
			default:
				throw syntax_error("Expected \'binary\' or \'unary\' keyword", current_token_->get_position());
			}
			break;
		default:
			throw syntax_error("Expected a function name or user-define operator", current_token_->get_position());
		}

		if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::LBRACKET)
			throw syntax_error("Expected '(' in prototype", current_token_->get_position());
		get_next_token_();

		std::vector<std::pair<std::string, llvm::Type *>> args;

		if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::RBRACKET)
		{
			while (true)
			{
				if (current_token_->get_type() != token_categories::IDENTIFIER)
					throw syntax_error("Expected a argument name in prototype", current_token_->get_position());
				auto arg_name = get_value<identifier>(current_token_);
				get_next_token_();

				if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::COLON)
					throw syntax_error("Expected a ':' after argument \'" + arg_name + "\'", current_token_->get_position());
				get_next_token_();

				if (current_token_->get_type() != token_categories::TYPE || get_value<type>(current_token_) == type_categories::VOID)
					throw syntax_error("Expected a correct type after argument \'" + arg_name + "\'", current_token_->get_position());
				llvm::Type * arg_type;
				switch (get_value<type>(current_token_))
				{
				case type_categories::NUMBER:
					arg_type = llvm::Type::getDoubleTy(llvm::getGlobalContext());
					break;
				default:
					throw syntax_error("Unknown type", current_token_->get_position());
				}
				get_next_token_();

				args.push_back(std::make_pair(arg_name, arg_type));

				if (current_token_->get_type() == token_categories::OPERATOR && get_value<op>(current_token_) == operator_categories::RBRACKET)
					break;

				if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::COMM)
					throw syntax_error("Expected a ')' or ',' in prototype", current_token_->get_position());

				get_next_token_();
			}
		}
		get_next_token_();

		if (current_token_->get_type() != token_categories::OPERATOR || get_value<op>(current_token_) != operator_categories::ARROW)
			throw syntax_error("Expected a return type of function \'" + name + "\'", current_token_->get_position());
		get_next_token_();

		if (current_token_->get_type() != token_categories::TYPE)
			throw syntax_error("Expected a return type of function \'" + name + "\'", current_token_->get_position());
		
		llvm::Type * ret_type;
		switch (get_value<type>(current_token_))
		{
		case type_categories::NUMBER:
			ret_type = llvm::Type::getDoubleTy(llvm::getGlobalContext());
			break;
		case type_categories::VOID:
			ret_type = llvm::Type::getVoidTy(llvm::getGlobalContext());
			break;
		default:
			throw syntax_error("Unknown type", current_token_->get_position());
		}
		get_next_token_();

		if (kind && kind != args.size())
			throw syntax_error("Invalid number of operands of operator", current_token_->get_position());

		return std::make_unique<prototype_ast>(name, std::move(args), ret_type,  kind != 0, precedence, start_row_no);
	}

	std::unique_ptr<function_ast> parser::parse_function_()
	{
		auto start_row_no = current_token_->get_position();
		get_next_token_();
		
		auto prototype = parse_prototype_();
		if (!prototype)
			return nullptr;

		auto body = parse_block_();
		if (!body)
			return nullptr;

		return std::make_unique<function_ast>(std::move(prototype), std::move(body), start_row_no);
	}

	std::unique_ptr<prototype_ast> parser::parse_extern_()
	{
		get_next_token_();

		return parse_prototype_();
	}

	std::unique_ptr<function_ast> parser::parse_top_level_expr_()
	{
		auto start_row_no = current_token_->get_position();
		auto prototype = std::make_unique<prototype_ast>("", std::vector<std::pair<std::string, llvm::Type *>>(), llvm::Type::getVoidTy(llvm::getGlobalContext()), false, -1, start_row_no);
		auto expression = parse_expression_();
		if (!expression)
			return nullptr;

		return std::make_unique<function_ast>(std::move(prototype), std::move(expression), start_row_no);
	}

	llvm::Value * number_ast::codegen()
	{
		return llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(value_));
	}

	llvm::Value * string_ast::codegen()
	{
		return global_builder.CreateGlobalStringPtr(value_);
	}

	llvm::Value * variable_ast::codegen()
	{
		auto ptr = global_named_values.find(name_);
		if(ptr == global_named_values.end())
			throw compile_error("Unknown variable name \'" + name_ + "\'", get_position());
		auto info = ptr->second;
		return global_builder.CreateLoad(info.second, info.first);
	}

	llvm::Value * binary_expression_ast::codegen()
	{
		auto l_value = left_->codegen();
		auto r_value = right_->codegen();

		switch (op_type_)
		{
		case operator_categories::ADD:
			return global_builder.CreateFAdd(l_value, r_value, "addtmp");
		case operator_categories::SUB:
			return global_builder.CreateFSub(l_value, r_value, "subtmp");
		case operator_categories::MUL:
			return global_builder.CreateFMul(l_value, r_value, "multmp");
		case operator_categories::DIV:
			return global_builder.CreateFDiv(l_value, r_value, "divtmp");
		case operator_categories::LT:
		{
			auto tmp = global_builder.CreateFCmpULT(l_value, r_value, "cmptmp");
			return global_builder.CreateUIToFP(tmp, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "booltmp");
		}
		case operator_categories::GT:
		{
			auto tmp = global_builder.CreateFCmpUGT(l_value, r_value, "cmptmp");
			return global_builder.CreateUIToFP(tmp, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "booltmp");
		}
		case operator_categories::LE:
		{
			auto tmp = global_builder.CreateFCmpULE(l_value, r_value, "cmptmp");
			return global_builder.CreateUIToFP(tmp, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "booltmp");
		}
		case operator_categories::GE:
		{
			auto tmp = global_builder.CreateFCmpUGE(l_value, r_value, "cmptmp");
			return global_builder.CreateUIToFP(tmp, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "booltmp");
		}
		case operator_categories::NEQ:
		{
			auto tmp = global_builder.CreateFCmpUNE(l_value, r_value, "cmptmp");
			return global_builder.CreateUIToFP(tmp, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "booltmp");
		}
		case operator_categories::EQ:
		{
			auto tmp = global_builder.CreateFCmpUEQ(l_value, r_value, "cmptmp");
			return global_builder.CreateUIToFP(tmp, llvm::Type::getDoubleTy(llvm::getGlobalContext()), "booltmp");
		}
		case operator_categories::ASSIGN:
		{
			auto tmp = dynamic_cast<variable_ast *>(left_.get());
			if (!tmp)
				throw compile_error("Destination of \'=\' must be a variable", get_position());
			auto ptr = global_named_values.find(tmp->get_name());
			if (ptr == global_named_values.end())
				throw syntax_error("Unknown variable name \'" + tmp->get_name() + "'", get_position());
			auto info = ptr->second;
			auto dest = info.first;
			global_builder.CreateStore(r_value, dest);
			return dest;
		}
		default:
			break;
		}

		auto function = global_JIT_helper->get_function("binary" + op_name_);
		if (!function)
			throw compile_error("Unknown operator", get_position());

		llvm::Value * args[] = { l_value, r_value };
		return global_builder.CreateCall(function, args, "binop");
	}

	llvm::Value * call_expression_ast::codegen()
	{
		auto callee_function = global_JIT_helper->get_function(callee_);
		if (!callee_function)
			throw compile_error("Unknown function referenced", get_position());

		if (callee_function->arg_size() != args_.size())
			throw compile_error("Incorrect number of arguments passed", get_position());

		std::vector<llvm::Value *> args_value;
		for (auto & arg : args_)
		{
			args_value.push_back(arg->codegen());
			if (!args_value.back())
				return nullptr;
		}

		return global_builder.CreateCall(callee_function, args_value);
	}
	
	llvm::Function * prototype_ast::codegen()
	{
		std::vector<llvm::Type *> args_type;
		for (auto & arg : args_)
			args_type.push_back(arg.second);

		auto function_type = llvm::FunctionType::get(ret_type_, args_type, false);
		auto module = global_JIT_helper->get_module_for_new_function();

		auto function_name = MCJIT_helper::generate_function_name(name_);
		auto function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, function_name, module);
		if (function->getName() != function_name)
		{
			function->eraseFromParent();
			function = global_JIT_helper->get_function(name_);
			if (!function->empty())
				throw compile_error("Redefinition of function " + name_, get_position());
			if (function->arg_size() != args_.size())
				throw compile_error("Redefinition of function " + name_ + " with different number of args", get_position());
		}

		auto id = 0;
		for (auto & arg : function->args())
			arg.setName(args_[id++].first);

		return function;
	}
	
	llvm::Function * function_ast::codegen()
	{
		global_named_values.clear();

		auto function = prototype_->codegen();
		if (!function)
			return nullptr;

		if (prototype_->is_binary_op())
			set_op_precedence(prototype_->get_operator_name(), prototype_->get_binary_op_precedence());

		auto basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry", function);
		global_builder.SetInsertPoint(basic_block);

		global_named_values.clear();
		for (auto & arg : function->args())
		{
			auto alloca_inst = global_create_alloca(function, arg.getName(), arg.getType());
			global_builder.CreateStore(&arg, alloca_inst);
			global_named_values[arg.getName()].first = alloca_inst;
			global_named_values[arg.getName()].second = arg.getType();
		}

		body_->codegen();
		if(function->getReturnType()->isVoidTy())
			global_builder.CreateRetVoid();

		llvm::verifyFunction(*function);
		return function;
	}

	llvm::Value * if_expression_ast::codegen()
	{
		auto cond_value = cond_->codegen();
		if (!cond_value)
			return nullptr;

		cond_value = global_builder.CreateFCmpONE(cond_value, llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0)), "ifcond");
		auto parent = global_builder.GetInsertBlock()->getParent();

		auto then_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "then", parent);
		auto else_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "else");
		auto merge_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "merge");

		global_builder.CreateCondBr(cond_value, then_basic_block, else_basic_block);
		global_builder.SetInsertPoint(then_basic_block);
		
		auto then_value = then_part_->codegen();
		if (!then_value)
			return nullptr;
		
		global_builder.CreateBr(merge_basic_block);
		then_basic_block = global_builder.GetInsertBlock();

		parent->getBasicBlockList().push_back(else_basic_block);
		global_builder.SetInsertPoint(else_basic_block);

		auto else_value = else_part_->codegen();
		if (!else_value)
			return nullptr;

		global_builder.CreateBr(merge_basic_block);
		else_basic_block = global_builder.GetInsertBlock();

		parent->getBasicBlockList().push_back(merge_basic_block);
		global_builder.SetInsertPoint(merge_basic_block);

		auto PHI_node = global_builder.CreatePHI(llvm::Type::getDoubleTy(llvm::getGlobalContext()), 2, "iftmp");
		PHI_node->addIncoming(then_value, then_basic_block);
		PHI_node->addIncoming(else_value, else_basic_block);
		return PHI_node;
	}

	llvm::Value * for_expression_ast::codegen()
	{
		auto parent = global_builder.GetInsertBlock()->getParent();
		auto alloca_inst = global_create_alloca(parent, var_name_, var_type_);

		auto old_val = global_named_values[var_name_];
		global_named_values[var_name_].first = alloca_inst;
		global_named_values[var_name_].second = var_type_;
		
		auto start_value = start_->codegen();
		if (!start_value)
			return nullptr;

		global_builder.CreateStore(start_value, alloca_inst);

		auto parent_basic_block = global_builder.GetInsertBlock();
		auto cmp_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "cmp", parent);
		auto body_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "body");
		auto after_basic_block = llvm::BasicBlock::Create(llvm::getGlobalContext(), "after");

		global_builder.CreateBr(cmp_basic_block);

		global_builder.SetInsertPoint(cmp_basic_block);	
		auto end_cond = end_->codegen();												  
		if (!end_cond)
			return false;

		end_cond = global_builder.CreateFCmpONE(end_cond, llvm::ConstantFP::get(llvm::getGlobalContext(), llvm::APFloat(0.0)), "loop_cond");
		global_builder.CreateCondBr(end_cond, body_basic_block, after_basic_block);
		cmp_basic_block = global_builder.GetInsertBlock();

		parent->getBasicBlockList().push_back(body_basic_block);
		global_builder.SetInsertPoint(body_basic_block);
		if (!body_->codegen())
			return nullptr;

		auto step_value = step_->codegen();
		if (!step_value)
			return nullptr;

		auto current_value = global_builder.CreateLoad(alloca_inst);
		auto next_value = global_builder.CreateFAdd(current_value, step_value, "next_var");
		global_builder.CreateStore(next_value, alloca_inst);
		global_builder.CreateBr(cmp_basic_block);
		body_basic_block = global_builder.GetInsertBlock();

		parent->getBasicBlockList().push_back(after_basic_block);
		global_builder.SetInsertPoint(after_basic_block);

		if (old_val.first)
			global_named_values[var_name_] = old_val;
		else
			global_named_values.erase(var_name_);
		after_basic_block = global_builder.GetInsertBlock();

		return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(llvm::getGlobalContext()));
	}

	llvm::Value * unary_expression_ast::codegen()
	{
		auto function = global_JIT_helper->get_function("unary" + op_name_);
		if (!function)
			throw compile_error("Unknown unary operator", get_position());

		auto operand = expr_->codegen();
		if (!operand)
			return nullptr;
		
		llvm::Value * args[] = { operand };
		return global_builder.CreateCall(function, args, "unaryop");
	}

	llvm::Value * var_ast::codegen()
	{
		std::vector<std::pair<llvm::AllocaInst *, llvm::Type *>> old_bindings;

		auto parent = global_builder.GetInsertBlock()->getParent();
		for (auto i = vars_.begin(); i != vars_.end(); ++i)
		{
			auto var_name = std::get<0>(*i);
			auto var_type = std::get<1>(*i);
			auto var_init = std::get<2>(*i).get();

			auto init_value = var_init->codegen();

			auto alloca_inst = global_create_alloca(parent, var_name, var_type);
			global_builder.CreateStore(init_value, alloca_inst);

			old_bindings.push_back(global_named_values[var_name]);
			global_named_values[var_name].first = alloca_inst;
			global_named_values[var_name].second = var_type;
		}

		auto body_value = body_->codegen();
		if (!body_value)
			return nullptr;

		for (auto i = 0; i != vars_.size(); ++i)
			global_named_values[std::get<0>(vars_[i])] = old_bindings[i];

		return body_value;
	}

	llvm::Value * block_ast::codegen()
	{
		for (auto & expr : exprs_)
			expr->codegen();

		return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(llvm::getGlobalContext()));
	}

	llvm::Value * return_ast::codegen()
	{
		return global_builder.CreateRet(ret_->codegen());
	}
	llvm::Value * empty_ast::codegen()
	{
		return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(llvm::getGlobalContext()));
	}
}