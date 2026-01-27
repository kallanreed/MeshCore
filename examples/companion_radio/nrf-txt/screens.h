#pragma once

#include <helpers/ui/UIScreen.h>
#include "UITask.h"

/*
  UIScreen API
  int render(DisplayDriver& display) override;
  virtual bool handleInput(char c) { return false; }
  virtual void poll() { }
*/

class SplashScreen : public UIScreen
{
  UITask* _task;
  unsigned long dismiss_after;
  char _version_info[12];

public:
  SplashScreen(UITask* task);
  int render(DisplayDriver& display) override;
  void poll() override;
};

class HomeScreen : public UIScreen
{
public:
  HomeScreen(UITask* task);
  int render(DisplayDriver& display) override;
  void poll() override;
};
