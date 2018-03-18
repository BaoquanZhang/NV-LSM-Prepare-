#include "mem_structure.h"

using namespace std;
using namespace nv_lsm;

/* Implementations for MemTable */
MemTable::MemTable() {}

MemTable::~MemTable() 
{
    delete(buffer);
}

/* Implementations for MetaTable */
MetaTable::MetaTable() {}
MetaTable::~MetaTable(){}
