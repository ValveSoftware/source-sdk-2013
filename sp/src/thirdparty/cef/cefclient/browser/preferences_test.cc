// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/browser/preferences_test.h"

#include <sstream>
#include <string>
#include <vector>

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"
#include "include/cef_parser.h"

namespace client {
namespace preferences_test {

namespace {

const char kTestUrl[] = "http://tests/preferences";

// Application-specific error codes.
const int kMessageFormatError = 1;
const int kPreferenceApplicationError = 1;

// Common to all messages.
const char kNameKey[] = "name";
const char kNameValueGet[] = "preferences_get";
const char kNameValueSet[] = "preferences_set";
const char kNameValueState[] = "preferences_state";

// Used with "preferences_get" messages.
const char kIncludeDefaultsKey[] = "include_defaults";

// Used with "preferences_set" messages.
const char kPreferencesKey[] = "preferences";

// Handle messages in the browser process. Only accessed on the UI thread.
class Handler : public CefMessageRouterBrowserSide::Handler {
 public:
  typedef std::vector<std::string> NameVector;

  Handler() {
    CEF_REQUIRE_UI_THREAD();
  }

  // Called due to cefQuery execution in preferences.html.
  bool OnQuery(CefRefPtr<CefBrowser> browser,
               CefRefPtr<CefFrame> frame,
               int64 query_id,
               const CefString& request,
               bool persistent,
               CefRefPtr<Callback> callback) OVERRIDE {
    CEF_REQUIRE_UI_THREAD();

    // Only handle messages from the test URL.
    std::string url = frame->GetURL();
    if (url.find(kTestUrl) != 0)
      return false;

    // Parse |request| as a JSON dictionary.
    CefRefPtr<CefDictionaryValue> request_dict = ParseJSON(request);
    if (!request_dict) {
      callback->Failure(kMessageFormatError, "Incorrect message format");
      return true;
    }

    // Verify the "name" key.
    if (!VerifyKey(request_dict, kNameKey, VTYPE_STRING, callback))
      return true;

    const std::string& message_name = request_dict->GetString(kNameKey);
    if (message_name == kNameValueGet) {
      // JavaScript is requesting a JSON representation of the preferences tree.

      // Verify the "include_defaults" key.
      if (!VerifyKey(request_dict, kIncludeDefaultsKey, VTYPE_BOOL, callback))
        return true;

      const bool include_defaults = request_dict->GetBool(kIncludeDefaultsKey);

      OnPreferencesGet(browser, include_defaults, callback);

      return true;
    } else if (message_name == kNameValueSet) {
      // JavaScript is requesting that preferences be updated to match the
      // specified JSON representation.

      // Verify the "preferences" key.
      if (!VerifyKey(request_dict, kPreferencesKey, VTYPE_DICTIONARY, callback))
        return true;

      CefRefPtr<CefDictionaryValue> preferences =
          request_dict->GetDictionary(kPreferencesKey);

      OnPreferencesSet(browser, preferences, callback);

      return true;
    } else if (message_name == kNameValueState) {
      // JavaScript is requesting global state information.

      OnPreferencesState(browser, callback);

      return true;
    }

    return false;
  }

 private:
  // Execute |callback| with the preferences dictionary as a JSON string.
  static void OnPreferencesGet(CefRefPtr<CefBrowser> browser,
                               bool include_defaults,
                               CefRefPtr<Callback> callback) {
    CefRefPtr<CefRequestContext> context =
        browser->GetHost()->GetRequestContext();

    // Retrieve all preference values.
    CefRefPtr<CefDictionaryValue> prefs =
        context->GetAllPreferences(include_defaults);

    // Serialize the preferences to JSON and return to the JavaScript caller.
    callback->Success(GetJSON(prefs));
  }

  // Set preferences based on the contents of |preferences|. Execute |callback|
  // with a descriptive result message.
  static void OnPreferencesSet(CefRefPtr<CefBrowser> browser,
                               CefRefPtr<CefDictionaryValue> preferences,
                               CefRefPtr<Callback> callback) {
    CefRefPtr<CefRequestContext> context =
        browser->GetHost()->GetRequestContext();

    CefRefPtr<CefValue> value = CefValue::Create();
    value->SetDictionary(preferences);

    std::string error;
    NameVector changed_names;

    // Apply preferences. This may result in errors.
    const bool success = ApplyPrefs(context, std::string(), value, error,
                                    changed_names);

    // Create a message that accurately represents the result.
    std::string message;
    if (!changed_names.empty()) {
      std::stringstream ss;
      ss << "Successfully changed " << changed_names.size() <<
            " preferences; ";
      for (size_t i = 0; i < changed_names.size(); ++i) {
        ss << changed_names[i];
        if (i < changed_names.size() - 1)
          ss << ", ";
      }
      message = ss.str();
    }

    if (!success) {
      DCHECK(!error.empty());
      if (!message.empty())
        message += "\n";
      message += error;
    }

    if (changed_names.empty()) {
      if (!message.empty())
        message += "\n";
      message += "No preferences changed.";
    }

    // Return the message to the JavaScript caller.
    if (success)
      callback->Success(message);
    else
      callback->Failure(kPreferenceApplicationError, message);
  }

  // Execute |callback| with the global state dictionary as a JSON string.
  static void OnPreferencesState(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<Callback> callback) {
    CefRefPtr<CefCommandLine> command_line =
        CefCommandLine::GetGlobalCommandLine();

    CefRefPtr<CefDictionaryValue> dict = CefDictionaryValue::Create();

    // If spell checking is disabled via the command-line then it cannot be
    // enabled via preferences.
    dict->SetBool("spellcheck_disabled",
        command_line->HasSwitch("disable-spell-checking"));

    // If proxy settings are configured via the command-line then they cannot
    // be modified via preferences.
    dict->SetBool("proxy_configured",
        command_line->HasSwitch("no-proxy-server") ||
        command_line->HasSwitch("proxy-auto-detect") ||
        command_line->HasSwitch("proxy-pac-url") ||
        command_line->HasSwitch("proxy-server"));

    // If allow running insecure content is enabled via the command-line then it
    // cannot be enabled via preferences.
    dict->SetBool("allow_running_insecure_content",
        command_line->HasSwitch("allow-running-insecure-content"));

    // Serialize the state to JSON and return to the JavaScript caller.
    callback->Success(GetJSON(dict));
  }

  // Convert a JSON string to a dictionary value.
  static CefRefPtr<CefDictionaryValue> ParseJSON(const CefString& string) {
    CefRefPtr<CefValue> value = CefParseJSON(string, JSON_PARSER_RFC);
    if (value.get() && value->GetType() == VTYPE_DICTIONARY)
      return value->GetDictionary();
    return NULL;
  }

  // Convert a dictionary value to a JSON string.
  static CefString GetJSON(CefRefPtr<CefDictionaryValue> dictionary) {
    CefRefPtr<CefValue> value = CefValue::Create();
    value->SetDictionary(dictionary);
    return CefWriteJSON(value, JSON_WRITER_DEFAULT);
  }

  // Verify that |key| exists in |dictionary| and has type |value_type|. Fails
  // |callback| and returns false on failure.
  static bool VerifyKey(CefRefPtr<CefDictionaryValue> dictionary,
                        const char* key,
                        cef_value_type_t value_type,
                        CefRefPtr<Callback> callback) {
    if (!dictionary->HasKey(key) || dictionary->GetType(key) != value_type) {
      callback->Failure(kMessageFormatError,
                        "Missing or incorrectly formatted message key: " +
                        std::string(key));
      return false;
    }
    return true;
  }

  // Apply preferences. Returns true on success. Returns false and sets |error|
  // to a descriptive error string on failure. |changed_names| is the list of
  // preferences that were successfully changed.
  static bool ApplyPrefs(CefRefPtr<CefRequestContext> context,
                         const std::string& name,
                         CefRefPtr<CefValue> value,
                         std::string& error,
                         NameVector& changed_names) {
    if (!name.empty() && context->HasPreference(name)) {
      // The preference exists. Set the value.
      return SetPref(context, name, value, error, changed_names);
    }

    if (value->GetType() == VTYPE_DICTIONARY) {
      // A dictionary type value that is not an existing preference. Try to set
      // each of the elements individually.
      CefRefPtr<CefDictionaryValue> dict = value->GetDictionary();

      CefDictionaryValue::KeyList keys;
      dict->GetKeys(keys);
      for (size_t i = 0; i < keys.size(); ++i) {
        const std::string& key = keys[i];
        const std::string& current_name =
            name.empty() ? key : name + "." + key;
        if (!ApplyPrefs(context, current_name, dict->GetValue(key), error,
                        changed_names)) {
          return false;
        }
      }

      return true;
    }

    error = "Trying to create an unregistered preference: " + name;
    return false;
  }

  // Set a specific preference value. Returns true if the value is set
  // successfully or has not changed. If the value has changed then |name| will
  // be added to |changed_names|. Returns false and sets |error| to a
  // descriptive error string on failure.
  static bool SetPref(CefRefPtr<CefRequestContext> context,
                      const std::string& name,
                      CefRefPtr<CefValue> value,
                      std::string& error,
                      NameVector& changed_names) {
    CefRefPtr<CefValue> existing_value = context->GetPreference(name);
    DCHECK(existing_value);

    if (value->GetType() == VTYPE_STRING &&
        existing_value->GetType() != VTYPE_STRING) {
      // Since |value| is coming from JSON all basic types will be represented
      // as strings. Convert to the expected data type.
      const std::string& string_val = value->GetString();
      switch (existing_value->GetType()) {
        case VTYPE_BOOL:
          if (string_val == "true" || string_val == "1")
            value->SetBool(true);
          else if (string_val == "false" || string_val == "0")
            value->SetBool(false);
          break;
        case VTYPE_INT:
          value->SetInt(atoi(string_val.c_str()));
          break;
        case VTYPE_DOUBLE:
          value->SetInt(atof(string_val.c_str()));
          break;
        default:
          // Other types cannot be converted.
          break;
      }
    }

    // Nothing to do if the value hasn't changed.
    if (existing_value->IsEqual(value))
      return true;

    // Attempt to set the preference.
    CefString error_str;
    if (!context->SetPreference(name, value, error_str)) {
      error = error_str.ToString() + ": " + name;
      return false;
    }

    // The preference was set successfully.
    changed_names.push_back(name);
    return true;
  }

  DISALLOW_COPY_AND_ASSIGN(Handler);
};

}  // namespace

void CreateMessageHandlers(test_runner::MessageHandlerSet& handlers) {
  handlers.insert(new Handler());
}

}  // namespace preferences_test
}  // namespace client
