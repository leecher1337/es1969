VOID FAR PASCAL fmwrite (WORD wAddress, BYTE bValue);
void FAR PASCAL fmreset();

SHORT NATV_CalcBend(USHORT detune, USHORT iBend, USHORT iBendRange);
BYTE NATV_CalcVolume(BYTE reg1, BYTE bVelocity, BYTE bChannel);
void NATV_CalcNewVolume(BYTE bChannel);

void note_on(BYTE bChannel, BYTE bNote, BYTE bVelocity);
void note_off(BYTE bChannel, BYTE bNote);

void voice_on(int voiceNr);
void voice_off(int voiceNr);

void hold_controller(BYTE bChannel, BYTE bVelocity);
void find_voice(BOOL patch1617_allowed_voice1, BOOL patch1617_allowed_voice2, BYTE bChannel, BYTE bNote);
void setup_voice(int voicenr, int offset, int bChannel, int bNote, int bVelocity);
int  steal_voice(int patch1617_allowed);

VOID MidiAllNotesOff(void);
WORD NEAR PASCAL MidiCalcFAndB (DWORD dwPitch, BYTE bBlock);
VOID NEAR PASCAL MidiPitchBend (BYTE  bChannel, USHORT iBend);
VOID NEAR PASCAL MidiMessage (DWORD dwData);

