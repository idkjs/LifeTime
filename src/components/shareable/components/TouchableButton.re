open ReactNative;
open ReactMultiversal;

let styles =
  Style.{
    "container":
      viewStyle(
        ~justifyContent=`center,
        ~alignItems=`center,
        ~borderRadius=Theme.Radius.button,
        (),
      ),
    "text": textStyle(~fontSize=18., ~lineHeight=18., ~fontWeight=`_600, ()),
  }
  ->StyleSheet.create;

[@react.component]
let make = (~onPress, ~text, ~styles as s=?) => {
  let theme = Theme.useTheme(AppSettings.useTheme());

  <TouchableOpacity onPress>
    <SpacedView
      vertical=S
      style=Style.(
        arrayOption([|
          Some(styles##container),
          Some(theme.styles##backgroundMain),
          s,
        |])
      )>
      <Text style=Style.(array([|styles##text, theme.styles##textOnMain|]))>
        text->React.string
      </Text>
    </SpacedView>
  </TouchableOpacity>;
};
