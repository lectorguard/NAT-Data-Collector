#pragma once

class TraversalWindow
{
public:
	void Activate(class Application* app);
	void Draw(Application* app);
	void Deactivate(class Application* app) {};

private:
	std::vector<ImVec4> button_colors{};
};
