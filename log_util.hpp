char action_to_char(int action, int card) {
  char c = (1 << 7) | (action << 3) | card;
  return c;
}
int char_to_action(char c) {
  int c2a1 = (c >> 3) & 7;
  int c2a2 = c & 7;
  return c2a1 * 10 + c2a2;
}
char wizard_to_char(int to, int trash, int draw) {
  char c = (1 << 7) | (to << 6) | (trash << 3) | draw;
  return c;
}
int char_to_wizard(char c) {
  int c2w1 = (c >> 6) & 1;
  int c2w2 = (c >> 3) & 7;
  int c2w3 = c & 7;
  return c2w1 * 100 + c2w2 * 10 + c2w3;
}
char twonum_to_char(int card1, int card2) {
  char c = (1 << 7) | (card1 << 3) | card2;
  return c;
}
int char_to_twonum(char c) {
  int c2t1 = (c >> 3) & 7;
  int c2t2 = c & 7;
  return c2t1 * 10 + c2t2;
}