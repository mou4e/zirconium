// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOM_DISTILLER_DOM_DISTILLER_SERVICE_FACTORY_H_
#define IOS_CHROME_BROWSER_DOM_DISTILLER_DOM_DISTILLER_SERVICE_FACTORY_H_

#include "components/dom_distiller/core/distilled_page_prefs.h"
#include "components/keyed_service/ios/browser_state_keyed_service_factory.h"

template <typename T> struct DefaultSingletonTraits;

class KeyedService;

namespace dom_distiller {
class DomDistillerService;
}

namespace web {
class BrowserState;
}

namespace dom_distiller {

class DomDistillerServiceFactory : public BrowserStateKeyedServiceFactory {
 public:
  static DomDistillerServiceFactory* GetInstance();
  static DomDistillerService* GetForBrowserState(
      web::BrowserState* browser_state);

 private:
  friend struct DefaultSingletonTraits<DomDistillerServiceFactory>;

  DomDistillerServiceFactory();
  ~DomDistillerServiceFactory() override;

  // BrowserStateKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      web::BrowserState* browser_state) const override;
  web::BrowserState* GetBrowserStateToUse(
      web::BrowserState* browser_state) const override;
};

}  // namespace dom_distiller

#endif  // IOS_CHROME_BROWSER_DOM_DISTILLER_DOM_DISTILLER_SERVICE_FACTORY_H_