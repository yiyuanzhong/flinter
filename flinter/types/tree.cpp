/* Copyright 2014 yiyuanzhong@gmail.com (Yiyuan Zhong)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flinter/types/tree.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <ostream>
#include <sstream>

#include "flinter/convert.h"

namespace flinter {

const Tree kDummyConstTree;

// TODO(yiyuanzhong): this object is in danger if multiple threads access it.
Tree kDummyTree;

#define DEFINE_AS(type,name) \
template <> \
type Tree::as(const type &defval) const \
{ \
    return convert_to_##name(_value, defval); \
}

// Don't define derived types like "uint64_t", template issue.
DEFINE_AS(         char       , char   );
DEFINE_AS(         short      , short  );
DEFINE_AS(         int        , int    );
DEFINE_AS(         long       , long   );
DEFINE_AS(         long long  , llong  );
DEFINE_AS(         float      , float  );
DEFINE_AS(         double     , double );
DEFINE_AS(         long double, ldouble);
DEFINE_AS(unsigned char       , uchar  );
DEFINE_AS(unsigned short      , ushort );
DEFINE_AS(unsigned int        , uint   );
DEFINE_AS(unsigned long       , ulong  );
DEFINE_AS(unsigned long long  , ullong );

template <>
bool Tree::as<bool>(const bool &defval) const
{
    return ParseBool(_value, defval);
}

template <>
bool Tree::key_as<bool>(const bool &defval) const
{
    return ParseBool(_key, defval);
}

bool Tree::ParseBool(const std::string &value, bool defval)
{
    std::string v(value);
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    if (v.compare("true") == 0 ||
        v.compare("1")    == 0 ){

        return true;

    } else if (v.compare("false") == 0 ||
               v.compare("0")     == 0 ){

        return false;

    } else {
        return defval;
    }
}

const char *Tree::as(const char *defval) const
{
    if (_value.empty()) {
        return defval;
    }
    return _value.c_str();
}

const std::string &Tree::as(const std::string &defval) const
{
    if (_value.empty()) {
        return defval;
    }
    return _value;
}

const char *Tree::key_as(const char *defval) const
{
    if (_key.empty()) {
        return defval;
    }
    return _key.c_str();
}

const std::string &Tree::key_as(const std::string &defval) const
{
    if (_key.empty()) {
        return defval;
    }
    return _key;
}

Tree::Tree(const std::string &full_path,
           const std::string &key,
           const std::string &value) : first(_key), second(_value)
                                     , _key(key)
                                     , _value(value)
                                     , _full_path(full_path)
{
    // Intended left blank.
}

Tree::Tree() : first(_key), second(_value)
{
    // Intended left blank.
}

Tree::~Tree()
{
    Clear();
}

Tree *Tree::CreateOrSet(const std::string &path, const std::string &value)
{
    if (path.empty()) { // Invalid path.
        return NULL;
    }

    std::string path_copy(path);
    char *cp = &path_copy[0];
    Tree *tree = this;
    while (true) {
        char *cr = strchr(cp, '.');
        if (cr == cp) { // Invalid path.
            return NULL;
        } else if (cr) {
            *cr = '\0';
        }

        std::string full_path = tree->GetFullPath(cp);

        std::map<std::string, Tree *>::const_iterator p = tree->_children.find(cp);
        if (p == tree->_children.end()) {
            Tree *node;
            if (!cr) { // Target node.
                node = new Tree(full_path, cp, value);
            } else { // Intermediate node.
                node = new Tree(full_path, cp, std::string());
            }

            p = tree->_children.insert(std::make_pair(cp, node)).first;
        }

        if (!cr) { // My own children.
            return p->second;
        }

        // Intermediate node.
        *cr = '.';
        cp = cr + 1;
        if (!*cp) { // Invalid path.
            return NULL;
        }

        tree = p->second;
    }
}

const Tree *Tree::Find(const std::string &path) const
{
    if (path.empty()) { // Invalid path.
        return NULL;
    }

    std::string path_copy(path);
    char *cp = &path_copy[0];
    const Tree *tree = this;
    while (true) {
        char *cr = strchr(cp, '.');
        if (cr == cp) { // Invalid path.
            return NULL;
        } else if (cr) {
            *cr = '\0';
        }

        std::map<std::string, Tree *>::const_iterator p = tree->_children.find(cp);
        if (p == tree->_children.end()) {
            return NULL;
        }

        if (!cr) { // My own children.
            return p->second;
        }

        // Intermediate node.
        cp = cr + 1;
        if (!*cp) { // Invalid path.
            return NULL;
        }

        tree = p->second;
    }
}

bool Tree::Has(const std::string &path) const
{
    return !!Find(path);
}

const Tree &Tree::Get(const std::string &path) const
{
    const Tree *tree = Find(path);
    if (tree) {
        return *tree;
    }

    return kDummyConstTree;
}

Tree &Tree::Get(const std::string &path)
{
    Tree *tree = CreateOrSet(path, std::string());
    if (tree) {
        return *tree;
    }

    return kDummyTree;
}

std::ostream &operator << (std::ostream &out, const Tree &tree)
{
    out << tree.value();
    return out;
}

std::string Tree::GetFullPath(const std::string &child) const
{
    if (_full_path.empty()) {
        return child;
    } else {
        return _full_path + "." + child;
    }
}

void Tree::Clear()
{
    _key.clear();
    _value.clear();

    // Don't clear _full_path.
    // _full_path.clear();

    for (std::map<std::string, Tree *>::iterator p = _children.begin();
         p != _children.end(); ++p) {

        delete p->second;
    }

    _children.clear();
}

bool Tree::RenderTemplateFile(const std::string &filename,
                              std::ostream *out,
                              std::string *error) const
{
    return RenderTemplateInternal(filename, true, out, error);
}

bool Tree::RenderTemplateString(const std::string &tmpl,
                                std::ostream *out,
                                std::string *error) const
{
    return RenderTemplateInternal(tmpl, false, out, error);
}

Tree::const_iterator Tree::begin() const
{
    const_iterator p;
    p._p = _children.begin();
    return p;
}

Tree::const_iterator Tree::end() const
{
    const_iterator p;
    p._p = _children.end();
    return p;
}

Tree::iterator Tree::begin()
{
    iterator p;
    p._p = _children.begin();
    return p;
}

Tree::iterator Tree::end()
{
    iterator p;
    p._p = _children.end();
    return p;
}

Tree &Tree::operator =(const Tree &other)
{
    if (this == &other) {
        return *this;
    }

    // It's very expensive to make a copy first, but if someone passes a subtree of this
    // very object in, we could destroy the subtree before copying.
    Tree copy(other);
    Clear();
    Merge(copy, true);
    return *this;
}

Tree::Tree(const Tree &other) : first(_key), second(_value)
{
    Merge(other, true);
}

void Tree::Merge(const Tree &other, bool overwrite_existing_nodes)
{
    if (this == &other) {
        return;
    }

    for (std::map<std::string, Tree *>::const_iterator p = other._children.begin();
         p != other._children.end(); ++p) {

        std::map<std::string, Tree *>::iterator q = _children.find(p->first);
        if (q == _children.end()) {
            const std::string &key = p->first;
            const std::string &value = p->second->_value;
            std::string full_path = _full_path + '.' + key;

            Tree *child = new Tree(full_path, key, value);
            q = _children.insert(std::make_pair(key, child)).first;
        }

        q->second->Merge(*p->second, overwrite_existing_nodes);
    }

    if (overwrite_existing_nodes) {
        _key = other._key;
        _value = other._value;

        // Don't copy _full_path.
        // _full_path = other._full_path;
    }
}

} // namespace flinter
