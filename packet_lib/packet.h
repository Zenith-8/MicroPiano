typedef struct {
    // Notes are stored by volume, values 0-127 denoting whether a note is played
    unsigned int note_volume[13];
    // Octave denoted by the individual ID of each hall effect board
    unsigned int octave;
    // Packet number for debugging, determining lost packets
    unsigned int packet_number;
 } Packet;