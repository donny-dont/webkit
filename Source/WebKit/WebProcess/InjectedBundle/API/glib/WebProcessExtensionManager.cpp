/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebProcessExtensionManager.h"

#include "APIString.h"
#include "InjectedBundle.h"
#include "WebKitWebProcessExtensionPrivate.h"
#include <memory>
#include <wtf/FileSystem.h>
#include <wtf/text/CString.h>

namespace WebKit {

WebProcessExtensionManager& WebProcessExtensionManager::singleton()
{
    static NeverDestroyed<WebProcessExtensionManager> extensionManager;
    return extensionManager;
}

void WebProcessExtensionManager::scanModules(const String& webProcessExtensionsDirectory, Vector<String>& modules)
{
    auto moduleNames = FileSystem::listDirectory(webProcessExtensionsDirectory);
    for (auto& moduleName : moduleNames) {
        if (!moduleName.endsWith(".so"_s))
            continue;

        auto modulePath = FileSystem::pathByAppendingComponent(webProcessExtensionsDirectory, moduleName);
        if (FileSystem::fileExists(modulePath))
            modules.append(modulePath);
    }
}

static void parseUserData(API::Object* userData, String& webProcessExtensionsDirectory, GRefPtr<GVariant>& initializationUserData)
{
    ASSERT(userData->type() == API::Object::Type::String);

    CString userDataString = downcast<API::String>(userData)->string().utf8();

    WTF_ALLOW_UNSAFE_BUFFER_USAGE_BEGIN // GTK/WPE port
    GRefPtr<GVariant> variant = adoptGRef(g_variant_parse(nullptr, userDataString.data(),
        userDataString.data() + userDataString.length(), nullptr, nullptr));
    WTF_ALLOW_UNSAFE_BUFFER_USAGE_END

    ASSERT(variant);
    ASSERT(g_variant_check_format_string(variant.get(), "(m&smv)", FALSE));

    const char* directory = nullptr;
    GVariant* data = nullptr;
    g_variant_get(variant.get(), "(m&smv)", &directory, &data);

    webProcessExtensionsDirectory = FileSystem::stringFromFileSystemRepresentation(directory);
    initializationUserData = adoptGRef(data);
}

bool WebProcessExtensionManager::initializeWebProcessExtension(Module* extensionModule, GVariant* userData)
{
#if ENABLE(2022_GLIB_API)
    WebKitWebProcessExtensionInitializeWithUserDataFunction initializeWithUserDataFunction =
        extensionModule->functionPointer<WebKitWebProcessExtensionInitializeWithUserDataFunction>("webkit_web_process_extension_initialize_with_user_data");
#else
    WebKitWebExtensionInitializeWithUserDataFunction initializeWithUserDataFunction =
        extensionModule->functionPointer<WebKitWebExtensionInitializeWithUserDataFunction>("webkit_web_extension_initialize_with_user_data");
#endif
    if (initializeWithUserDataFunction) {
        initializeWithUserDataFunction(m_extension.get(), userData);
        return true;
    }

#if ENABLE(2022_GLIB_API)
    WebKitWebProcessExtensionInitializeFunction initializeFunction =
        extensionModule->functionPointer<WebKitWebProcessExtensionInitializeFunction>("webkit_web_process_extension_initialize");
#else
    WebKitWebExtensionInitializeFunction initializeFunction =
        extensionModule->functionPointer<WebKitWebExtensionInitializeFunction>("webkit_web_extension_initialize");
#endif
    if (initializeFunction) {
        initializeFunction(m_extension.get());
        return true;
    }

    return false;
}

void WebProcessExtensionManager::initialize(InjectedBundle* bundle, API::Object* userDataObject)
{
    ASSERT(bundle);
    ASSERT(userDataObject);
    m_extension = adoptGRef(webkitWebProcessExtensionCreate(bundle));

    String webProcessExtensionsDirectory;
    GRefPtr<GVariant> userData;
    parseUserData(userDataObject, webProcessExtensionsDirectory, userData);

    if (webProcessExtensionsDirectory.isNull())
        return;

    Vector<String> modulePaths;
    scanModules(webProcessExtensionsDirectory, modulePaths);

    for (size_t i = 0; i < modulePaths.size(); ++i) {
        auto module = makeUnique<Module>(modulePaths[i]);
        if (!module->load())
            continue;
        if (initializeWebProcessExtension(module.get(), userData.get()))
            m_extensionModules.append(module.release());
    }
}

} // namespace WebKit
