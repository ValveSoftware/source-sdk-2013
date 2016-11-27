// Copyright (c) 2014 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_LIBCEF_DLL_WRAPPER_CEF_BROWSER_INFO_MAP_H_
#define CEF_LIBCEF_DLL_WRAPPER_CEF_BROWSER_INFO_MAP_H_
#pragma once

#include <map>

#include "include/base/cef_logging.h"
#include "include/base/cef_macros.h"

// Default traits for CefBrowserInfoMap. Override to provide different object
// destruction behavior.
template<typename ObjectType>
struct DefaultCefBrowserInfoMapTraits {
  static void Destruct(ObjectType info) {
    delete info;
  }
};

// Maps an arbitrary IdType to an arbitrary ObjectType on a per-browser basis.
template <typename IdType,
          typename ObjectType,
          typename Traits = DefaultCefBrowserInfoMapTraits<ObjectType> >
class CefBrowserInfoMap {
 public:
  // Implement this interface to visit and optionally delete objects in the map.
  class Visitor {
   public:
    typedef IdType InfoIdType;
    typedef ObjectType InfoObjectType;

    // Called once for each info object. Set |remove| to true to remove the
    // object from the map. It is safe to destruct removed objects in this
    // callback. Return true to continue iterating or false to stop iterating.
    virtual bool OnNextInfo(int browser_id,
                            InfoIdType info_id,
                            InfoObjectType info,
                            bool* remove) =0;

   protected:
    virtual ~Visitor() {}
  };

  CefBrowserInfoMap() {}

  ~CefBrowserInfoMap() {
    clear();
  }

  // Add an object associated with the specified ID values.
  void Add(int browser_id, IdType info_id, ObjectType info) {
    InfoMap* info_map = NULL;
    typename BrowserInfoMap::const_iterator it_browser =
        browser_info_map_.find(browser_id);
    if (it_browser == browser_info_map_.end()) {
      // No InfoMap exists for the browser ID so create it.
      info_map = new InfoMap;
      browser_info_map_.insert(std::make_pair(browser_id, info_map));
    } else {
      info_map = it_browser->second;
      // The specified ID should not already exist in the map.
      DCHECK(info_map->find(info_id) == info_map->end());
    }

    info_map->insert(std::make_pair(info_id, info));
  }

  // Find the object with the specified ID values. |visitor| can optionally be
  // used to evaluate or remove the object at the same time. If the object is
  // removed using the Visitor the caller is responsible for destroying it.
  ObjectType Find(int browser_id, IdType info_id, Visitor* vistor) {
    if (browser_info_map_.empty())
      return ObjectType();

    typename BrowserInfoMap::iterator it_browser =
        browser_info_map_.find(browser_id);
    if (it_browser == browser_info_map_.end())
      return ObjectType();

    InfoMap* info_map = it_browser->second;
    typename InfoMap::iterator it_info = info_map->find(info_id);
    if (it_info == info_map->end())
      return ObjectType();

    ObjectType info = it_info->second;

    bool remove = false;
    if (vistor)
      vistor->OnNextInfo(browser_id, it_info->first, info, &remove);
    if (remove) {
      info_map->erase(it_info);

      if (info_map->empty()) {
        // No more entries in the InfoMap so remove it.
        browser_info_map_.erase(it_browser);
        delete info_map;
      }
    }

    return info;
  }

  // Find all objects. If any objects are removed using the Visitor the caller
  // is responsible for destroying them.
  void FindAll(Visitor* visitor) {
    DCHECK(visitor);

    if (browser_info_map_.empty())
      return;

    bool remove, keepgoing = true;

    typename BrowserInfoMap::iterator it_browser = browser_info_map_.begin();
    while (it_browser != browser_info_map_.end()) {
      InfoMap* info_map = it_browser->second;

      typename InfoMap::iterator it_info = info_map->begin();
      while (it_info != info_map->end()) {
        remove = false;
        keepgoing = visitor->OnNextInfo(it_browser->first, it_info->first,
                                        it_info->second, &remove);

        if (remove)
          info_map->erase(it_info++);
        else
          ++it_info;

        if (!keepgoing)
          break;
      }

      if (info_map->empty()) {
        // No more entries in the InfoMap so remove it.
        browser_info_map_.erase(it_browser++);
        delete info_map;
      } else {
        ++it_browser;
      }

      if (!keepgoing)
        break;
    }
  }

  // Find all objects associated with the specified browser. If any objects are
  // removed using the Visitor the caller is responsible for destroying them.
  void FindAll(int browser_id, Visitor* visitor) {
    DCHECK(visitor);

    if (browser_info_map_.empty())
      return;

    typename BrowserInfoMap::iterator it_browser =
        browser_info_map_.find(browser_id);
    if (it_browser == browser_info_map_.end())
      return;

    InfoMap* info_map = it_browser->second;
    bool remove, keepgoing;

    typename InfoMap::iterator it_info = info_map->begin();
    while (it_info != info_map->end()) {
      remove = false;
      keepgoing = visitor->OnNextInfo(browser_id, it_info->first,
                                      it_info->second, &remove);

      if (remove)
        info_map->erase(it_info++);
      else
        ++it_info;

      if (!keepgoing)
        break;
    }

    if (info_map->empty()) {
      // No more entries in the InfoMap so remove it.
      browser_info_map_.erase(it_browser);
      delete info_map;
    }
  }

  // Returns true if the map is empty.
  bool empty() const { return browser_info_map_.empty(); }

  // Returns the number of objects in the map.
  size_t size() const {
    if (browser_info_map_.empty())
      return 0;

    size_t size = 0;
    typename BrowserInfoMap::const_iterator it_browser =
        browser_info_map_.begin();
    for (; it_browser != browser_info_map_.end(); ++it_browser)
      size += it_browser->second->size();
    return size;
  }

  // Returns the number of objects in the map that are associated with the
  // specified browser.
  size_t size(int browser_id) const {
    if (browser_info_map_.empty())
      return 0;

    typename BrowserInfoMap::const_iterator it_browser =
        browser_info_map_.find(browser_id);
    if (it_browser != browser_info_map_.end())
      return it_browser->second->size();

    return 0;
  }

  // Remove all objects from the map. The objects will be destructed.
  void clear() {
    if (browser_info_map_.empty())
      return;

    typename BrowserInfoMap::const_iterator it_browser =
        browser_info_map_.begin();
    for (; it_browser != browser_info_map_.end(); ++it_browser) {
      InfoMap* info_map = it_browser->second;
      typename InfoMap::const_iterator it_info = info_map->begin();
      for (; it_info != info_map->end(); ++it_info)
        Traits::Destruct(it_info->second);
      delete info_map;
    }
    browser_info_map_.clear();
  }

  // Remove all objects from the map that are associated with the specified
  // browser. The objects will be destructed.
  void clear(int browser_id) {
    if (browser_info_map_.empty())
      return;

    typename BrowserInfoMap::iterator it_browser =
        browser_info_map_.find(browser_id);
    if (it_browser == browser_info_map_.end())
      return;

    InfoMap* info_map = it_browser->second;
    typename InfoMap::const_iterator it_info = info_map->begin();
    for (; it_info != info_map->end(); ++it_info)
      Traits::Destruct(it_info->second);

    browser_info_map_.erase(it_browser);
    delete info_map;
  }

 private:
  // Map IdType to ObjectType instance.
  typedef std::map<IdType, ObjectType> InfoMap;
  // Map browser ID to InfoMap instance.
  typedef std::map<int, InfoMap*> BrowserInfoMap;

  BrowserInfoMap browser_info_map_;

  DISALLOW_COPY_AND_ASSIGN(CefBrowserInfoMap);
};


#endif  // CEF_LIBCEF_DLL_WRAPPER_CEF_BROWSER_INFO_MAP_H_
