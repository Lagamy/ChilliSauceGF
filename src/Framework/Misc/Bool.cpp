#include "Bool.h"


Bool::Bool()
{
	this->value = 0u;
}

Bool::Bool(bool value_)
{
	this->value = value_ ? 1u : 0u;
}

Bool& Bool::operator=(bool value_)
{
	this->value = value_ ? 1u : 0u;
	return *this;
}

Bool::operator bool() const
{
	return this->value != 0u;
}