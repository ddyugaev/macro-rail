#include "PhotoMode.h"

PhotoMode::PhotoMode()
  : ModeEnc(F("Start"))
{
  m_message.reserve(10);
}

void PhotoMode::initImpl() {
  g_settings.finalize();
  m_addition = 0;
  m_minPosition = 0;
  m_maxPosition = 0;
  m_fromPosition = 0;
  m_frameDepth = g_settings.toPosition > 0 ? g_settings.photoSettings.frameDepth : -g_settings.photoSettings.frameDepth;
  m_nFrames = g_settings.nFrames;

  m_currentStage = Stage::stDone;
  m_framesShot = 0;
  m_message = "";

  g_stepper.enable();
  display();
  nextStage();
}

void PhotoMode::updateImpl() {
  ModeEnc::updateImpl();
  if (m_currentStage == stDone)
    return;

  if (m_currentStageWorker->inProgress())
    return;

  String errorMsg;
  if (m_currentStageWorker->isBroken(errorMsg)) {
    stopProcess(errorMsg);
  } else {
    nextStage();
  }
}

void PhotoMode::nextStage() {
  m_currentStage = (m_currentStage) + 1;
  switch(m_currentStage) {
    case stInit:
      m_currentStage = stCountMove;
      m_currentStageWorker = &initStage;
      {
        long targetPos = m_fromPosition + g_stepper.mmToSteps(m_frameDepth*m_nFrames);
        initStage.start(m_fromPosition, targetPos, g_settings.photoSettings.initSec, m_nFrames);
      }
      break;
    case stLoop:
      m_currentStage = stCalm;
      [[fallthrough]];
    case stCalm:
      g_camera.focus();
      m_currentStageWorker = &timeoutStage;
      timeoutStage.start(g_settings.photoSettings.calmMsec);
      break;
    case stExposure:
      updateMinMax();
      m_currentStageWorker = &exposureStage;
      exposureStage.start(g_settings.photoSettings.exposureMsec);
      break;
    case stCountMove:
      m_framesShot++;
      if (m_framesShot >= m_nFrames) {
        stopProcess("Done!");
      } else {
        m_currentStageWorker = &moveStage;
        moveStage.start(m_frameDepth);
      }
      display();
      break;
    default:
      break;
  }
}

void PhotoMode::stopProcess(const String & reason) {
  g_camera.release();
  m_currentStageWorker->cancel();
  m_currentStage = stDone;
  if (reason.length() > 0)
    m_message = reason;
  display();
}

void PhotoMode::updateMinMax() {
  if (m_frameDepth > 0) {
    m_maxPosition = g_stepper.getPosition();
  } else {
    m_minPosition = g_stepper.getPosition();
  }
}

void PhotoMode::onClick() {
  if (m_currentStage == stDone) {
    // Return to menu immediately when stopped
    ModeEnc::onClick();
  } else {
    stopProcess("Stop");
  }
}

void PhotoMode::onTurn(int dir) {
  // No action when stopped - encoder turns are ignored
}

void PhotoMode::display() {
  String msg;
  msg.reserve(16);
  msg += m_framesShot;
  msg += " of ";
  msg += m_nFrames;

  if (m_message.length() > 0) {
    msg += ": ";
    msg += m_message;
  }

  displayValue(msg);
}

