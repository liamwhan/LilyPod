//
//  wav_gui.h
//  vtwav
//
//  Created by Liam Whan on 12/4/21.
//

#if !defined(WAV_GUI_H)
#define WAV_GUI_H

void InitUI(ui_state *State);
void PushButtonTextColour();
void PopButtonTextColour();
ui_state InitUiState();
void RenderUi(ui_state *State);
void ShutdownUi();

#define RGB_TO_IMVEC4F(r, g, b) (ImVec4)ImColor((float) r / 255.f, (float) g / 255.f, (float) b / 255.f)

#endif /* WAV_GUI_H */
