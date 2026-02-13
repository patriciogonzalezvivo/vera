#include "vera/app.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Primitives2DApp : public vera::App {
public:
    void setup() override {
        background(0.15f);
        textSize(14.0f);
    }

    void draw() override {
        background(0.15f);

        // Clear default 3D camera so shapes use the built-in pixel-coordinate ortho matrix
        resetCamera();

        float colW = (float)width / 3.0f;
        float rowH = (float)height / 4.0f;

        // Row 1: Rectangular shapes
        // -- Square (red)
        float cx = colW * 0.5f;
        float cy = rowH * 0.5f;
        fill(0.9f, 0.2f, 0.2f);
        stroke(1.0f);
        strokeWeight(2.0f);
        square(cx - 40.0f, cy - 40.0f, 80.0f);
        fill(1.0f);
        noStroke();
        text("square()", cx - 30.0f, cy + 55.0f);

        // -- Quad (green, trapezoid)
        cx = colW * 1.5f;
        fill(0.2f, 0.8f, 0.2f);
        stroke(1.0f);
        strokeWeight(2.0f);
        quad(cx - 30.0f, cy - 40.0f,
             cx + 30.0f, cy - 40.0f,
             cx + 50.0f, cy + 40.0f,
             cx - 50.0f, cy + 40.0f);
        fill(1.0f);
        noStroke();
        text("quad()", cx - 22.0f, cy + 55.0f);

        // -- Rect (blue, reference)
        cx = colW * 2.5f;
        fill(0.2f, 0.3f, 0.9f);
        stroke(1.0f);
        strokeWeight(2.0f);
        rect(cx - 50.0f, cy - 30.0f, 100.0f, 60.0f);
        fill(1.0f);
        noStroke();
        text("rect()", cx - 22.0f, cy + 55.0f);

        // Row 2: Round shapes
        // -- Ellipse (yellow)
        cx = colW * 0.75f;
        cy = rowH * 1.5f;
        fill(0.9f, 0.9f, 0.2f);
        stroke(1.0f);
        strokeWeight(2.0f);
        ellipse(cx, cy, 120.0f, 70.0f);
        fill(1.0f);
        noStroke();
        text("ellipse()", cx - 30.0f, cy + 50.0f);

        // -- Circle (cyan, reference)
        cx = colW * 2.25f;
        fill(0.2f, 0.9f, 0.9f);
        stroke(1.0f);
        strokeWeight(2.0f);
        circle(cx, cy, 45.0f);
        fill(1.0f);
        noStroke();
        text("circle()", cx - 26.0f, cy + 60.0f);

        // Row 3: Arcs
        float arcW = 90.0f;
        float arcH = 90.0f;
        cy = rowH * 2.5f;

        // -- PIE_MODE (magenta)
        cx = colW * 0.5f;
        fill(0.9f, 0.2f, 0.9f);
        stroke(1.0f);
        strokeWeight(2.0f);
        arc(cx, cy, arcW, arcH, 0.0f, (float)(M_PI * 1.25), vera::PIE_MODE);
        fill(1.0f);
        noStroke();
        text("arc PIE", cx - 26.0f, cy + 60.0f);

        // -- CHORD_MODE (orange)
        cx = colW * 1.5f;
        fill(1.0f, 0.6f, 0.1f);
        stroke(1.0f);
        strokeWeight(2.0f);
        arc(cx, cy, arcW, arcH, 0.0f, (float)(M_PI * 1.25), vera::CHORD_MODE);
        fill(1.0f);
        noStroke();
        text("arc CHORD", cx - 34.0f, cy + 60.0f);

        // -- OPEN_MODE (white fill, white stroke)
        cx = colW * 2.5f;
        fill(0.7f);
        stroke(1.0f);
        strokeWeight(2.0f);
        arc(cx, cy, arcW, arcH, 0.0f, (float)(M_PI * 1.25), vera::OPEN_MODE);
        fill(1.0f);
        noStroke();
        text("arc OPEN", cx - 30.0f, cy + 60.0f);

        // Row 4: Triangle
        cx = (float)width * 0.5f;
        cy = rowH * 3.5f;
        fill(0.5f, 0.8f, 0.3f);
        stroke(1.0f);
        strokeWeight(2.0f);
        triangle(cx - 50.0f, cy + 35.0f,
                 cx + 50.0f, cy + 35.0f,
                 cx, cy - 35.0f);
        fill(1.0f);
        noStroke();
        text("triangle()", cx - 34.0f, cy + 55.0f);
    }
};

int main(int argc, char** argv) {
    Primitives2DApp app;
    vera::WindowProperties props;
    props.screen_width = 1024;
    props.screen_height = 768;
    app.run(props);
    return 0;
}
