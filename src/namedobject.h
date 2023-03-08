#pragma once
#include <string>
#include <utility>
#include "typedef.h"
namespace protolang
{

class NamedObject
{
public:
	/// 第一个该具名实体生效的token的位置
	std::string name;

	struct Properties
	{
		Pos2DRange available_pos;
		Pos2DRange ident_pos;
	} props;

public:
	explicit NamedObject(const Properties &props, std::string name)
	    : props(props)
	    , name(std::move(name))
	{}
	virtual ~NamedObject()                = default;
	virtual std::string dump_json() const = 0;
};

class NamedVar : public NamedObject
{
public:
	std::string type;

	NamedVar(const Properties &props, std::string name, std::string type)
	    : NamedObject(props, std::move(name))

	    , type(std::move(type))
	{}
	virtual std::string dump_json() const
	{
		return std::format(R"({{ "name": "{}", "type": "{}" }})", name, type);
	}
};

class NamedFunc : public NamedObject
{
public:
	std::string           return_type;
	std::vector<NamedVar> params;

	NamedFunc(const Properties       &props,
	          string                  name,
	          string                  returnType,
	          const vector<NamedVar> &params)
	    : NamedObject(props, std::move(name))
	    , return_type(std::move(returnType))
	    , params(params)
	{}
	virtual std::string dump_json() const
	{
		return std::format(
		    R"({{ "name": "{}", "return": "{}", "params": {} }})",
		    name,
		    return_type,
		    dump_json_for_vector(params));
	}
};

} // namespace protolang