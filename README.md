# interactor-healthkit-osc

Send Apple Watch heart rate to VRChat as OSC, from your own phone, with no cloud
account.

An iPhone app reads heart-rate samples from HealthKit and sends them as OSC over
UDP. One stream goes to VRChat as avatar parameters. The other goes to the
character controller, with a confidence. The messages are described in
[spec/asyncapi.yaml](spec/asyncapi.yaml).

## Limits

- It is a phone app. The watch must run a workout for live samples, and each
  sample arrives a few seconds late.
- The app sends the heart rate and never chooses a gesture or an emote.

## Licence

MIT. See [LICENSE](LICENSE).
