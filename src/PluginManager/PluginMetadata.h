#ifndef Kompas_Plugins_PluginMetadata_h
#define Kompas_Plugins_PluginMetadata_h
/*
    Copyright © 2007, 2008, 2009, 2010, 2011 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Kompas.

    Kompas is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Kompas is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

/** @file
 * @brief Class Kompas::PluginManager::PluginMetadata
 */

#include <string>
#include <vector>
#include <map>

#include "Utility/Configuration.h"
#include "Utility/Translator.h"

namespace Kompas { namespace PluginManager {

/**
 * @brief %Plugin metadata
 *
 * This class stores metadata about particular plugin.
 * See also @ref PluginManagement.
 */
class PLUGINMANAGER_EXPORT PluginMetadata {
    friend class AbstractPluginManager;

    public:
        /**
         * @brief Constructor
         * @param conf          Configuration file with plugin metadata
         */
        PluginMetadata(const Utility::Configuration& conf);

        /**
         * @brief %Plugin name
         *
         * Descriptive name of plugin. Not to be confused with name under which
         * the plugin is loaded. If translation for current Translator::locale()
         * is present, returns the translated name.
         * @note This field is constant during whole plugin lifetime.
         */
        inline const std::string* name() const { return _name; }

        /**
         * @brief %Plugin description
         *
         * More detailed description of plugin. If translation for current
         * Translator::locale() is present, returns the translated name.
         * @note This field is constant during whole plugin lifetime.
         */
        inline const std::string* description() const { return _description; }

        /**
         * @brief %Plugin author(s)
         *
         * @note This field is constant during whole plugin lifetime.
         */
        inline const std::vector<std::string>& authors() const { return _authors; }

        /**
         * @brief %Plugin version
         *
         * @note This field is constant during whole plugin lifetime.
         */
        inline std::string version() const { return _version; }

        /**
         * @brief Plugins on which this plugin depend
         *
         * List of plugins which must be loaded before this plugin can be
         * loaded. See also PluginMetadata::replaced.
         * @note Thus field is constant during whole plugin lifetime.
         */
        inline const std::vector<std::string>& depends() const { return _depends; }

        /**
         * @brief Plugins which depend on this plugin
         *
         * List of plugins which uses this plugin. This plugin cannot be
         * unloaded when any of these plugins are loaded.
         * @note This list is automatically created by plugin manager and can
         *      be changed in plugin lifetime.
         */
        inline std::vector<std::string> usedBy() const { return _usedBy; }

        /**
         * @brief Plugins which are replaced with this plugin
         *
         * Plugins which depends on them can be used with this plugin. The
         * plugin cannot be loaded when any of the replaced plugins are loaded.
         * @note Thus field is constant during whole plugin lifetime.
         */
        inline const std::vector<std::string>& replaces() const { return _replaces; }

        /**
         * @brief Plugins which replaces this plugin
         *
         * List of plugins which can replace this plugin. Every plugin which
         * depends on this plugin would work also with these.
         * @note This list is automatically created by plugin manage and can
         *      change in plugin lifetime.
         */
        inline std::vector<std::string> replacedWith() const { return _replacedWith; }

    protected:
        /**
         * @brief Add plugin into "used by" list
         * @param name      %Plugin name
         */
        void addUsedBy(const std::string& name);

        /**
         * @brief Remove plugin from "used by" list
         * @param name      %Plugin name
         */
        void removeUsedBy(const std::string& name);

    private:
        Utility::Translator translator;

        const std::string *_name,
            *_description;
        std::string _version;

        std::vector<std::string> _authors,
            _depends,
            _usedBy,
            _replaces,
            _replacedWith;
};

}}

#endif
