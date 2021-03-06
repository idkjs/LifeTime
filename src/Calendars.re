open Belt;
open ReactNativeCalendarEvents;

let date0 =
  Js.Date.makeWithYMDHM(
    ~year=0.,
    ~month=0.,
    ~date=0.,
    ~hours=0.,
    ~minutes=0.,
    (),
  );

let sort = (calendars: array(calendar)) =>
  calendars->SortArray.stableSortBy((a, b) =>
    a.title > b.title ? 1 : a.title < b.title ? (-1) : 0
  );

let availableCalendars =
    (calendars: array(calendar), settings: AppSettings.t) =>
  calendars
  ->Array.keep(c =>
      !settings.calendarsIdsSkipped->Array.some(cs => cs == c.id)
    )
  ->Array.map(c => c.id);

let useCalendars = updater => {
  let (value, set) = React.useState(() => None);
  React.useEffect2(
    () => {
      findCalendars()
      ->FutureJs.fromPromise(error => {
          // @todo ?
          Js.log(error);
          error;
        })
      ->Future.tapOk(res => set(_ => Some(res->sort)))
      ->ignore;
      None;
    },
    (set, updater),
  );
  value;
};

type events = option(array(calendarEventReadable));
let defaultContext: (
  (Js.Date.t, Js.Date.t, bool) => events,
  Js.Date.t,
  unit => unit,
) = (
  (_, _, _) => None,
  Js.Date.make(),
  _ => (),
);
let context = React.createContext(defaultContext);

module ContextProvider = {
  let makeProps = (~value, ~children, ()) => {
    "value": value,
    "children": children,
  };
  let make = React.Context.provider(context);
};

let makeMapKey = (startDate, endDate) =>
  startDate->Js.Date.toISOString ++ endDate->Js.Date.toISOString;

let useEvents = () => {
  let (updatedAt, setUpdatedAt) = React.useState(_ => Date.now());
  let (eventsMapByRange, setEventsMapByRange) =
    React.useState(() => Map.String.empty);

  let requestUpdate =
    React.useCallback1(
      () => {
        setUpdatedAt(_ => Date.now());
        setEventsMapByRange(_ => Map.String.empty);
      },
      [|setUpdatedAt|],
    );

  let getEvents =
    React.useCallback2(
      (startDate, endDate, allowFetch) => {
        let res =
          eventsMapByRange->Map.String.get(makeMapKey(startDate, endDate));
        if (res->Option.isNone) {
          let res =
            eventsMapByRange->Map.String.get(makeMapKey(startDate, endDate));
          if (res->Option.isNone && allowFetch) {
            setEventsMapByRange(eventsMapByRange => {
              eventsMapByRange->Map.String.set(
                makeMapKey(startDate, endDate),
                None,
              )
            });
            fetchAllEvents(
              startDate->Js.Date.toISOString,
              endDate->Js.Date.toISOString,
              // we filter calendar later cause if you UNSELECT ALL
              // this `fetchAllEvents` DEFAULT TO ALL
              None,
            )
            ->FutureJs.fromPromise(error => {
                // @todo ?
                Js.log(error);
                error;
              })
            ->Future.tapOk(res => {
                setEventsMapByRange(eventsMapByRange =>
                  eventsMapByRange->Map.String.set(
                    makeMapKey(startDate, endDate),
                    Some(res),
                  )
                )
              })
            ->ignore;
            ();
          };
        };
        res->Option.flatMap(res => res);
      },
      (eventsMapByRange, setEventsMapByRange),
    );

  (getEvents, updatedAt, requestUpdate);
};

let filterEvents =
    (events: array(calendarEventReadable), settings: AppSettings.t) =>
  events->Array.keep(evt
    // filters out all day events
    =>
      if (evt.allDay->Option.getWithDefault(false)) {
        false;
             // filters selected calendars
      } else if (settings.calendarsIdsSkipped
                 ->Array.some(cid =>
                     cid
                     == evt.calendar
                        ->Option.map(c => c.id)
                        ->Option.getWithDefault("")
                   )) {
        false;
      } else if (settings.activitiesSkippedFlag
                 && settings.activitiesSkipped
                    ->Array.some(skipped =>
                        Activities.isSimilar(skipped, evt.title)
                      )) {
        false;
      } else {
        true;
      }
    );

let mapTitleDuration = (events: array(calendarEventReadable)) => {
  events
  ->Array.reduce(
      Map.String.empty,
      (map, evt) => {
        let key = evt.title->Activities.minifyName;
        map->Map.String.set(
          key,
          map
          ->Map.String.get(key)
          ->Option.getWithDefault([||])
          ->Array.concat([|evt|]),
        );
      },
    )
  ->Map.String.toArray
  ->Array.map(((_key, evts: array(calendarEventReadable))) => {
      let totalDurationInMin =
        evts->Array.reduce(
          0.,
          (totalDurationInMin, evt) => {
            let durationInMs =
              Date.durationInMs(
                evt.endDate->Js.Date.fromString,
                evt.startDate->Js.Date.fromString,
              );
            totalDurationInMin
            +. durationInMs->Js.Date.fromFloat->Js.Date.valueOf->Date.msToMin;
          },
        );
      (
        evts[0]->Option.map(evt => evt.title)->Option.getWithDefault(""),
        totalDurationInMin,
      );
    })
  ->SortArray.stableSortBy(((_, minA), (_, minB)) =>
      minA > minB ? (-1) : minA < minB ? 1 : 0
    );
};

let categoryIdFromActivityTitle = (settings: AppSettings.t, activityName) => {
  let activity =
    settings.activities
    ->Array.keep(acti =>
        Activities.isSimilar(acti.title, activityName)
        && acti.categoryId->ActivityCategories.isIdValid
      )[0]
    ->Option.getWithDefault(Activities.unknown);
  activity.categoryId;
};

let mapCategoryDuration =
    (events: array(calendarEventReadable), settings: AppSettings.t) => {
  events
  ->Array.reduce(
      Map.String.empty,
      (map, evt) => {
        let key = settings->categoryIdFromActivityTitle(evt.title);
        map->Map.String.set(
          key,
          map
          ->Map.String.get(key)
          ->Option.getWithDefault([||])
          ->Array.concat([|evt|]),
        );
      },
    )
  ->Map.String.toArray
  ->Array.map(((key, evts: array(calendarEventReadable))) => {
      let totalDurationInMin =
        evts->Array.reduce(
          0.,
          (totalDurationInMin, evt) => {
            let durationInMs =
              Date.durationInMs(
                evt.endDate->Js.Date.fromString,
                evt.startDate->Js.Date.fromString,
              );
            totalDurationInMin
            +. durationInMs->Js.Date.fromFloat->Js.Date.valueOf->Date.msToMin;
          },
        );
      (key, totalDurationInMin);
    })
  ->SortArray.stableSortBy(((_, minA), (_, minB)) =>
      minA > minB ? (-1) : minA < minB ? 1 : 0
    );
};
