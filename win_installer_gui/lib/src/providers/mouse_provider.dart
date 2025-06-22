import 'package:flutter/material.dart';

class MouseProvider extends ChangeNotifier {
  final List<Function()> _mouseListeners = [];

  void addMouseListener(Function() listener) {
    _mouseListeners.add(listener);
  }

  void removeMouseListener(Function() listener) {
    _mouseListeners.remove(listener);
  }

  void notifyMouseMove() {
    for (var listener in _mouseListeners) {
      listener();
    }
  }
} 