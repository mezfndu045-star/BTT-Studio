#pragma once

namespace btt {

// Photo tour used to take the sub area pictures: started by sd:/BTT-Studio/tour_start.txt (first shot index), it warps to
// each shot of PhotoTourList.h, waits for the stage to settle, writes the shot name to sd:/BTT-Studio/tour_shot.txt and
// waits for sd:/BTT-Studio/tour_ack.txt (written by the capture script once the screenshot is taken).
// sd:/BTT-Studio/tour_stop.txt stops it, sd:/BTT-Studio/tour_done.txt is written at the end.
void updatePhotoTour();
bool isPhotoTourRunning();

} // namespace btt
