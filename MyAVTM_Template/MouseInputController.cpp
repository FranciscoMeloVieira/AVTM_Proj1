#include "MouseInputController.h"
#include "PathEditor.h"

void MouseInputController::startRecording(PathEditor& editor, double timestampSeconds)
{
	editor.setInputMethod("mouse");
	editor.beginStroke(m_lastMouseWorld, timestampSeconds);
	m_recording = editor.getState() == EditorState::Recording;
}

void MouseInputController::stopRecording(PathEditor& editor)
{
	if (m_recording || editor.getState() == EditorState::Recording)
	{
		editor.endStroke();
	}
	m_recording = false;
}

void MouseInputController::toggleRecording(PathEditor& editor, double timestampSeconds)
{
	if (m_recording || editor.getState() == EditorState::Recording)
		stopRecording(editor);
	else
		startRecording(editor, timestampSeconds);
}

void MouseInputController::update(float deltaTimeSeconds, double currentTimestampSeconds, PathEditor& editor)
{
	(void)deltaTimeSeconds;
	(void)currentTimestampSeconds;
	m_recording = editor.getState() == EditorState::Recording;
}

void MouseInputController::onMouseDown(int screenX, int screenY, int windowWidth, int windowHeight,
	double timestampSeconds, PathEditor& editor)
{
	m_lastMouseWorld = PathEditor::screenToWorldOrtho(screenX, screenY, windowWidth, windowHeight, 200.0f, 150.0f, 0.0f);
	editor.setInputMethod("mouse");
	editor.beginStroke(m_lastMouseWorld, timestampSeconds);
	m_recording = editor.getState() == EditorState::Recording;
}

void MouseInputController::onMouseDrag(int screenX, int screenY, int windowWidth, int windowHeight,
	double timestampSeconds, PathEditor& editor)
{
	m_lastMouseWorld = PathEditor::screenToWorldOrtho(screenX, screenY, windowWidth, windowHeight, 200.0f, 150.0f, 0.0f);
	if (editor.getState() == EditorState::Recording)
	{
		editor.addSample(m_lastMouseWorld, timestampSeconds);
	}
}

void MouseInputController::onMouseUp(int screenX, int screenY, int windowWidth, int windowHeight,
	double timestampSeconds, PathEditor& editor)
{
	(void)screenX;
	(void)screenY;
	(void)windowWidth;
	(void)windowHeight;
	(void)timestampSeconds;
	if (editor.getState() == EditorState::Recording)
	{
		editor.endStroke();
	}
	m_recording = false;
}
