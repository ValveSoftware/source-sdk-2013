// Copyright (c) 2010 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "include/wrapper/cef_xml_object.h"

#include <sstream>

#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"
#include "include/cef_stream.h"

namespace {

class CefXmlObjectLoader {
 public:
  explicit CefXmlObjectLoader(CefRefPtr<CefXmlObject> root_object)
    : root_object_(root_object) {
  }

  bool Load(CefRefPtr<CefStreamReader> stream,
            CefXmlReader::EncodingType encodingType,
            const CefString& URI) {
    CefRefPtr<CefXmlReader> reader(
        CefXmlReader::Create(stream, encodingType, URI));
    if (!reader.get())
      return false;

    bool ret = reader->MoveToNextNode();
    if (ret) {
      CefRefPtr<CefXmlObject> cur_object(root_object_), new_object;
      CefXmlObject::ObjectVector queue;
      int cur_depth, value_depth = -1;
      CefXmlReader::NodeType cur_type;
      std::stringstream cur_value;
      bool last_has_ns = false;

      queue.push_back(root_object_);

      do {
        cur_depth = reader->GetDepth();
        if (value_depth >= 0 && cur_depth > value_depth) {
          // The current node has already been parsed as part of a value.
          continue;
        }

        cur_type = reader->GetType();
        if (cur_type == XML_NODE_ELEMENT_START) {
          if (cur_depth == value_depth) {
            // Add to the current value.
            cur_value << std::string(reader->GetOuterXml());
            continue;
          } else if (last_has_ns && reader->GetPrefix().empty()) {
            if (!cur_object->HasChildren()) {
              // Start a new value because the last element has a namespace and
              // this element does not.
              value_depth = cur_depth;
              cur_value << std::string(reader->GetOuterXml());
            } else {
              // Value following a child element is not allowed.
              std::stringstream ss;
              ss << "Value following child element, line " <<
                  reader->GetLineNumber();
              load_error_ = ss.str();
              ret = false;
              break;
            }
          } else {
            // Start a new element.
            new_object = new CefXmlObject(reader->GetQualifiedName());
            cur_object->AddChild(new_object);
            last_has_ns = !reader->GetPrefix().empty();

            if (!reader->IsEmptyElement()) {
              // The new element potentially has a value and/or children, so
              // set the current object and add the object to the queue.
              cur_object = new_object;
              queue.push_back(cur_object);
            }

            if (reader->HasAttributes() && reader->MoveToFirstAttribute()) {
              // Read all object attributes.
              do {
                new_object->SetAttributeValue(reader->GetQualifiedName(),
                    reader->GetValue());
              } while (reader->MoveToNextAttribute());
              reader->MoveToCarryingElement();
            }
          }
        } else if (cur_type == XML_NODE_ELEMENT_END) {
          if (cur_depth == value_depth) {
            // Ending an element that is already in the value.
            continue;
          } else if (cur_depth < value_depth) {
            // Done with parsing the value portion of the current element.
            cur_object->SetValue(cur_value.str());
            cur_value.str("");
            value_depth = -1;
          }

          // Pop the current element from the queue.
          queue.pop_back();

          if (queue.empty() ||
              cur_object->GetName() != reader->GetQualifiedName()) {
            // Open tag without close tag or close tag without open tag should
            // never occur (the parser catches this error).
            NOTREACHED();
            std::stringstream ss;
            ss << "Mismatched end tag for " <<
                std::string(cur_object->GetName()) <<
                ", line " << reader->GetLineNumber();
            load_error_ = ss.str();
            ret = false;
            break;
          }

          // Set the current object to the previous object in the queue.
          cur_object = queue.back().get();
        } else if (cur_type == XML_NODE_TEXT || cur_type == XML_NODE_CDATA ||
                   cur_type == XML_NODE_ENTITY_REFERENCE) {
          if (cur_depth == value_depth) {
            // Add to the current value.
            cur_value << std::string(reader->GetValue());
          } else if (!cur_object->HasChildren()) {
            // Start a new value.
            value_depth = cur_depth;
            cur_value << std::string(reader->GetValue());
          } else {
            // Value following a child element is not allowed.
            std::stringstream ss;
            ss << "Value following child element, line " <<
                reader->GetLineNumber();
            load_error_ = ss.str();
            ret = false;
            break;
          }
        }
      } while (reader->MoveToNextNode());
    }

    if (reader->HasError()) {
      load_error_ = reader->GetError();
      return false;
    }

    return ret;
  }

  CefString GetLoadError() { return load_error_; }

 private:
  CefString load_error_;
  CefRefPtr<CefXmlObject> root_object_;

  DISALLOW_COPY_AND_ASSIGN(CefXmlObjectLoader);
};

}  // namespace

CefXmlObject::CefXmlObject(const CefString& name)
  : name_(name), parent_(NULL) {
}

CefXmlObject::~CefXmlObject() {
}

bool CefXmlObject::Load(CefRefPtr<CefStreamReader> stream,
                        CefXmlReader::EncodingType encodingType,
                        const CefString& URI, CefString* loadError) {
  Clear();

  CefXmlObjectLoader loader(this);
  if (!loader.Load(stream, encodingType, URI)) {
    if (loadError)
      *loadError = loader.GetLoadError();
    return false;
  }
  return true;
}

void CefXmlObject::Set(CefRefPtr<CefXmlObject> object) {
  DCHECK(object.get());

  Clear();

  name_ = object->GetName();
  Append(object, true);
}

void CefXmlObject::Append(CefRefPtr<CefXmlObject> object,
                          bool overwriteAttributes) {
  DCHECK(object.get());

  if (object->HasChildren()) {
    ObjectVector children;
    object->GetChildren(children);
    ObjectVector::const_iterator it = children.begin();
    for (; it != children.end(); ++it)
      AddChild((*it)->Duplicate());
  }

  if (object->HasAttributes()) {
    AttributeMap attributes;
    object->GetAttributes(attributes);
    AttributeMap::const_iterator it = attributes.begin();
    for (; it != attributes.end(); ++it) {
      if (overwriteAttributes || !HasAttribute(it->first))
        SetAttributeValue(it->first, it->second);
    }
  }
}

CefRefPtr<CefXmlObject> CefXmlObject::Duplicate() {
  CefRefPtr<CefXmlObject> new_obj;
  {
    base::AutoLock lock_scope(lock_);
    new_obj = new CefXmlObject(name_);
    new_obj->Append(this, true);
  }
  return new_obj;
}

void CefXmlObject::Clear() {
  ClearChildren();
  ClearAttributes();
}

CefString CefXmlObject::GetName() {
  CefString name;
  {
    base::AutoLock lock_scope(lock_);
    name = name_;
  }
  return name;
}

bool CefXmlObject::SetName(const CefString& name) {
  DCHECK(!name.empty());
  if (name.empty())
    return false;

  base::AutoLock lock_scope(lock_);
  name_ = name;
  return true;
}

bool CefXmlObject::HasParent() {
  base::AutoLock lock_scope(lock_);
  return (parent_ != NULL);
}

CefRefPtr<CefXmlObject> CefXmlObject::GetParent() {
  CefRefPtr<CefXmlObject> parent;
  {
    base::AutoLock lock_scope(lock_);
    parent = parent_;
  }
  return parent;
}

bool CefXmlObject::HasValue() {
  base::AutoLock lock_scope(lock_);
  return !value_.empty();
}

CefString CefXmlObject::GetValue() {
  CefString value;
  {
    base::AutoLock lock_scope(lock_);
    value = value_;
  }
  return value;
}

bool CefXmlObject::SetValue(const CefString& value) {
  base::AutoLock lock_scope(lock_);
  DCHECK(children_.empty());
  if (!children_.empty())
    return false;
  value_ = value;
  return true;
}

bool CefXmlObject::HasAttributes() {
  base::AutoLock lock_scope(lock_);
  return !attributes_.empty();
}

size_t CefXmlObject::GetAttributeCount() {
  base::AutoLock lock_scope(lock_);
  return attributes_.size();
}

bool CefXmlObject::HasAttribute(const CefString& name) {
  if (name.empty())
    return false;

  base::AutoLock lock_scope(lock_);
  AttributeMap::const_iterator it = attributes_.find(name);
  return (it != attributes_.end());
}

CefString CefXmlObject::GetAttributeValue(const CefString& name) {
  DCHECK(!name.empty());
  CefString value;
  if (!name.empty()) {
    base::AutoLock lock_scope(lock_);
    AttributeMap::const_iterator it = attributes_.find(name);
    if (it != attributes_.end())
      value = it->second;
  }
  return value;
}

bool CefXmlObject::SetAttributeValue(const CefString& name,
                                     const CefString& value) {
  DCHECK(!name.empty());
  if (name.empty())
    return false;

  base::AutoLock lock_scope(lock_);
  AttributeMap::iterator it = attributes_.find(name);
  if (it != attributes_.end()) {
    it->second = value;
  } else {
    attributes_.insert(std::make_pair(name, value));
  }
  return true;
}

size_t CefXmlObject::GetAttributes(AttributeMap& attributes) {
  base::AutoLock lock_scope(lock_);
  attributes = attributes_;
  return attributes_.size();
}

void CefXmlObject::ClearAttributes() {
  base::AutoLock lock_scope(lock_);
  attributes_.clear();
}

bool CefXmlObject::HasChildren() {
  base::AutoLock lock_scope(lock_);
  return !children_.empty();
}

size_t CefXmlObject::GetChildCount() {
  base::AutoLock lock_scope(lock_);
  return children_.size();
}

bool CefXmlObject::HasChild(CefRefPtr<CefXmlObject> child) {
  DCHECK(child.get());

  base::AutoLock lock_scope(lock_);
  ObjectVector::const_iterator it = children_.begin();
  for (; it != children_.end(); ++it) {
    if ((*it).get() == child.get())
      return true;
  }
  return false;
}

bool CefXmlObject::AddChild(CefRefPtr<CefXmlObject> child) {
  DCHECK(child.get());
  if (!child.get())
    return false;

  CefRefPtr<CefXmlObject> parent = child->GetParent();
  DCHECK(!parent);
  if (parent)
    return false;

  base::AutoLock lock_scope(lock_);

  children_.push_back(child);
  child->SetParent(this);
  return true;
}

bool CefXmlObject::RemoveChild(CefRefPtr<CefXmlObject> child) {
  DCHECK(child.get());

  base::AutoLock lock_scope(lock_);
  ObjectVector::iterator it = children_.begin();
  for (; it != children_.end(); ++it) {
    if ((*it).get() == child.get()) {
      children_.erase(it);
      child->SetParent(NULL);
      return true;
    }
  }
  return false;
}

size_t CefXmlObject::GetChildren(ObjectVector& children) {
  base::AutoLock lock_scope(lock_);
  children = children_;
  return children_.size();
}

void CefXmlObject::ClearChildren() {
  base::AutoLock lock_scope(lock_);
  ObjectVector::iterator it = children_.begin();
  for (; it != children_.end(); ++it)
    (*it)->SetParent(NULL);
  children_.clear();
}

CefRefPtr<CefXmlObject> CefXmlObject::FindChild(const CefString& name) {
  DCHECK(!name.empty());
  if (name.empty())
    return NULL;

  base::AutoLock lock_scope(lock_);
  ObjectVector::const_iterator it = children_.begin();
  for (; it != children_.end(); ++it) {
    if ((*it)->GetName() == name)
      return (*it);
  }
  return NULL;
}

size_t CefXmlObject::FindChildren(const CefString& name,
                                  ObjectVector& children) {
  DCHECK(!name.empty());
  if (name.empty())
    return 0;

  size_t ct = 0;

  base::AutoLock lock_scope(lock_);
  ObjectVector::const_iterator it = children_.begin();
  for (; it != children_.end(); ++it) {
    if ((*it)->GetName() == name) {
      children.push_back(*it);
      ct++;
    }
  }
  return ct;
}

void CefXmlObject::SetParent(CefXmlObject* parent) {
  base::AutoLock lock_scope(lock_);
  if (parent) {
    DCHECK(parent_ == NULL);
    parent_ = parent;
  } else {
    DCHECK(parent_ != NULL);
    parent_ = NULL;
  }
}
