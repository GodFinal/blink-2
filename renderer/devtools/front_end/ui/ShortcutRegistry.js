// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @unrestricted
 */
UI.ShortcutRegistry = class {
  /**
   * @param {!UI.ActionRegistry} actionRegistry
   * @param {!Document} document
   */
  constructor(actionRegistry, document) {
    this._actionRegistry = actionRegistry;
    /** @type {!Multimap.<string, string>} */
    this._defaultKeyToActions = new Multimap();
    /** @type {!Multimap.<string, !UI.KeyboardShortcut.Descriptor>} */
    this._defaultActionToShortcut = new Multimap();
    this._registerBindings(document);
  }

  /**
   * @param {number} key
   * @return {!Array.<!UI.Action>}
   */
  _applicableActions(key) {
    return this._actionRegistry.applicableActions(this._defaultActionsForKey(key).valuesArray(), UI.context);
  }

  /**
   * @param {number} key
   * @return {!Set.<string>}
   */
  _defaultActionsForKey(key) {
    return this._defaultKeyToActions.get(String(key));
  }

  /**
   * @param {string} actionId
   * @return {!Array.<!UI.KeyboardShortcut.Descriptor>}
   */
  shortcutDescriptorsForAction(actionId) {
    return this._defaultActionToShortcut.get(actionId).valuesArray();
  }

  /**
   * @param {!Array.<string>} actionIds
   * @return {!Array.<number>}
   */
  keysForActions(actionIds) {
    const result = [];
    for (let i = 0; i < actionIds.length; ++i) {
      const descriptors = this.shortcutDescriptorsForAction(actionIds[i]);
      for (let j = 0; j < descriptors.length; ++j)
        result.push(descriptors[j].key);
    }
    return result;
  }

  /**
   * @param {string} actionId
   * @return {string|undefined}
   */
  shortcutTitleForAction(actionId) {
    const descriptors = this.shortcutDescriptorsForAction(actionId);
    if (descriptors.length)
      return descriptors[0].name;
  }

  /**
   * @param {!KeyboardEvent} event
   */
  handleShortcut(event) {
    this.handleKey(UI.KeyboardShortcut.makeKeyFromEvent(event), event.key, event);
  }

  /**
   * @param {!KeyboardEvent} event
   * @param {string} actionId
   * @return {boolean}
   */
  eventMatchesAction(event, actionId) {
    console.assert(this._defaultActionToShortcut.has(actionId), 'Unknown action ' + actionId);
    const key = UI.KeyboardShortcut.makeKeyFromEvent(event);
    return this._defaultActionToShortcut.get(actionId).valuesArray().some(descriptor => descriptor.key === key);
  }

  /**
   * @param {!Element} element
   * @param {string} actionId
   * @param {function():boolean} listener
   * @param {boolean=} capture
   */
  addShortcutListener(element, actionId, listener, capture) {
    console.assert(this._defaultActionToShortcut.has(actionId), 'Unknown action ' + actionId);
    element.addEventListener('keydown', event => {
      if (!this.eventMatchesAction(/** @type {!KeyboardEvent} */ (event), actionId) || !listener.call(null))
        return;
      event.consume(true);
    }, capture);
  }

  /**
   * @param {number} key
   * @param {string} domKey
   * @param {!KeyboardEvent=} event
   */
  handleKey(key, domKey, event) {
    const keyModifiers = key >> 8;
    const actions = this._applicableActions(key);
    if (!actions.length)
      return;
    if (UI.Dialog.hasInstance()) {
      if (event && !isPossiblyInputKey())
        event.consume(true);
      return;
    }

    if (!isPossiblyInputKey()) {
      if (event)
        event.consume(true);
      processNextAction.call(this, false);
    } else {
      this._pendingActionTimer = setTimeout(processNextAction.bind(this, false), 0);
    }

    /**
     * @param {boolean} handled
     * @this {UI.ShortcutRegistry}
     */
    function processNextAction(handled) {
      delete this._pendingActionTimer;
      const action = actions.shift();
      if (!action || handled)
        return;

      action.execute().then(processNextAction.bind(this));
    }

    /**
     * @return {boolean}
     */
    function isPossiblyInputKey() {
      if (!event || !UI.isEditing() || /^F\d+|Control|Shift|Alt|Meta|Escape|Win|U\+001B$/.test(domKey))
        return false;

      if (!keyModifiers)
        return true;

      const modifiers = UI.KeyboardShortcut.Modifiers;
      if ((keyModifiers & (modifiers.Ctrl | modifiers.Alt)) === (modifiers.Ctrl | modifiers.Alt))
        return Host.isWin();

      return !hasModifier(modifiers.Ctrl) && !hasModifier(modifiers.Alt) && !hasModifier(modifiers.Meta);
    }

    /**
     * @param {number} mod
     * @return {boolean}
     */
    function hasModifier(mod) {
      return !!(keyModifiers & mod);
    }
  }

  /**
   * @param {string} actionId
   * @param {string} shortcut
   */
  registerShortcut(actionId, shortcut) {
    const descriptor = UI.KeyboardShortcut.makeDescriptorFromBindingShortcut(shortcut);
    if (!descriptor)
      return;
    this._defaultActionToShortcut.set(actionId, descriptor);
    this._defaultKeyToActions.set(String(descriptor.key), actionId);
  }

  dismissPendingShortcutAction() {
    if (this._pendingActionTimer) {
      clearTimeout(this._pendingActionTimer);
      delete this._pendingActionTimer;
    }
  }

  /**
   * @param {!Document} document
   */
  _registerBindings(document) {
    document.addEventListener('input', this.dismissPendingShortcutAction.bind(this), true);
    const extensions = self.runtime.extensions('action');
    extensions.forEach(registerExtension, this);

    /**
     * @param {!Runtime.Extension} extension
     * @this {UI.ShortcutRegistry}
     */
    function registerExtension(extension) {
      const descriptor = extension.descriptor();
      const bindings = descriptor['bindings'];
      for (let i = 0; bindings && i < bindings.length; ++i) {
        if (!platformMatches(bindings[i].platform))
          continue;
        const shortcuts = bindings[i]['shortcut'].split(/\s+/);
        shortcuts.forEach(this.registerShortcut.bind(this, descriptor['actionId']));
      }
    }

    /**
     * @param {string=} platformsString
     * @return {boolean}
     */
    function platformMatches(platformsString) {
      if (!platformsString)
        return true;
      const platforms = platformsString.split(',');
      let isMatch = false;
      const currentPlatform = Host.platform();
      for (let i = 0; !isMatch && i < platforms.length; ++i)
        isMatch = platforms[i] === currentPlatform;
      return isMatch;
    }
  }
};

/**
 * @unrestricted
 */
UI.ShortcutRegistry.ForwardedShortcut = class {};

UI.ShortcutRegistry.ForwardedShortcut.instance = new UI.ShortcutRegistry.ForwardedShortcut();

/** @type {!UI.ShortcutRegistry} */
UI.shortcutRegistry;
