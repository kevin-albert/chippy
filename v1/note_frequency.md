- Lowest A is 13.75 (not audible)
- real_note = int representing number of half-steps above A<sub>13.75</sub>
- octave = real_note / 12
- note = real_note % 12
- Actual expression:
`13.75 * pow(2, octave + (float) note / 12)`
