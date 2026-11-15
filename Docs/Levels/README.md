# Level design documentation

This folder holds level blockout plans and editor notes. These documents describe proposed layouts; they do not change the Unreal maps or establish new gameplay. Unreal map assets remain under `Content/Maps/`.

## Home farm

- [Home farm plan](./HomeFarm/README.md) describes the full walkable farm layout, player routes, landmarks, scale survey, and Editor build order.
- [Home farm layout diagram](./HomeFarm/HomeFarm.svg) is an editable, relative-scale 2.5D master plan, not a survey of the current map.

The existing map is `/Game/Maps/L_Farm`. Verify actor locations and dimensions in the Editor before treating the diagram as a measured plan. Keep experimental changes in a duplicate map until the layout is agreed.

## Shared level-authoring tools

- [Reusable level tools](./Scripts/README.md) describes map-independent Editor survey, blockout generation, optional adjustment widgets, and route validation. These are implementation plans; no scripts are implemented yet.
