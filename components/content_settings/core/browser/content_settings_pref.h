// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_PREF_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_PREF_H_

#include <string>

#include "base/callback.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "components/content_settings/core/browser/content_settings_origin_identifier_value_map.h"
#include "components/content_settings/core/browser/content_settings_provider.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"

class PrefService;
class PrefChangeRegistrar;

namespace base {
class Clock;
class DictionaryValue;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace content_settings {

class RuleIterator;

// Represents a single pref for reading/writing content settings of one type.
class ContentSettingsPref {
 public:
  typedef base::Callback<void(const ContentSettingsPattern&,
                              const ContentSettingsPattern&,
                              ContentSettingsType,
                              const std::string&)> NotifyObserversCallback;

  ContentSettingsPref(ContentSettingsType content_type,
                      PrefService* prefs,
                      PrefChangeRegistrar* registrar,
                      const char* pref_name,
                      bool incognito,
                      bool* updating_old_preferences_flag,
                      NotifyObserversCallback notify_callback);
  ~ContentSettingsPref();

  RuleIterator* GetRuleIterator(const ResourceIdentifier& resource_identifier,
                                bool incognito) const;

  bool SetWebsiteSetting(const ContentSettingsPattern& primary_pattern,
                         const ContentSettingsPattern& secondary_pattern,
                         const ResourceIdentifier& resource_identifier,
                         base::Value* value);

  void ClearAllContentSettingsRules();

  void UpdateLastUsage(const ContentSettingsPattern& primary_pattern,
                       const ContentSettingsPattern& secondary_pattern,
                       base::Clock* clock);

  base::Time GetLastUsage(const ContentSettingsPattern& primary_pattern,
                          const ContentSettingsPattern& secondary_pattern);

  size_t GetNumExceptions();

 private:
  // Only to access static method CanonicalizeContentSettingsExceptions,
  // so that we reduce duplicity between the two.
  // TODO(msramek): Remove this after the migration is over.
  friend class PrefProvider;

  // Reads all content settings exceptions from the preference and load them
  // into the |value_map_|. The |value_map_| is cleared first.
  void ReadContentSettingsFromPrefAndWriteToOldPref();

  // Callback for changes in the pref with the same name.
  void OnPrefChanged();

  // Update the preference that stores content settings exceptions and syncs the
  // value to the obsolete preference. When calling this function, |lock_|
  // should not be held, since this function will send out notifications of
  // preference changes.
  void UpdatePref(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      const ResourceIdentifier& resource_identifier,
      const base::Value* value);

  static void CanonicalizeContentSettingsExceptions(
      base::DictionaryValue* all_settings_dictionary);

  // In the debug mode, asserts that |lock_| is not held by this thread. It's
  // ok if some other thread holds |lock_|, as long as it will eventually
  // release it.
  void AssertLockNotHeld() const;

  // Update the old aggregate preference, so that the settings can be synced
  // to old versions of Chrome.
  // TODO(msramek): Remove after the migration is over.
  void UpdateOldPref(
      const ContentSettingsPattern& primary_pattern,
      const ContentSettingsPattern& secondary_pattern,
      const ResourceIdentifier& resource_identifier,
      const base::Value* value);

  // Remove all exceptions of |content_type_| from the old aggregate dictionary
  // preference.
  // TODO(msramek): Remove after the migration is over.
  void ClearOldPreference();

  // The type of content settings stored in this pref.
  ContentSettingsType content_type_;

  // Weak; owned by the Profile and reset in ShutdownOnUIThread.
  PrefService* prefs_;

  // Owned by the PrefProvider.
  PrefChangeRegistrar* registrar_;

  // Name of the dictionary preference managed by this class.
  const char* pref_name_;

  bool is_incognito_;

  // Whether we are currently updating preferences, this is used to ignore
  // notifications from the preferences service that we triggered ourself.
  bool updating_preferences_;

  // Whether we are currently updating the old aggregate dictionary preference.
  // Owned by the parent |PrefProvider| and shared by all its children
  // |ContentSettingsPref|s.
  bool* updating_old_preferences_;

  OriginIdentifierValueMap value_map_;

  OriginIdentifierValueMap incognito_value_map_;

  NotifyObserversCallback notify_callback_;

  // Used around accesses to the value map objects to guarantee thread safety.
  mutable base::Lock lock_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsPref);
};

}  // namespace content_settings

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_CONTENT_SETTINGS_PREF_H_