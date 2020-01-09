open ReactNative;
open ReactMultiversal;

[@react.component]
let make = (~navigation, ~route as _) => {
  let themeStyles = Theme.useStyles();
  let themeColors = Theme.useColors();

  let scrollYAnimatedValue = React.useRef(Animated.Value.create(0.));
  <>
    <StatusBar barStyle=`lightContent />
    <Animated.ScrollView
      style={Style.list([
        Predefined.styles##flexGrow,
        themeStyles##backgroundDark,
      ])}
      showsHorizontalScrollIndicator=false
      showsVerticalScrollIndicator=false
      scrollEventThrottle=16
      onScroll=Animated.(
        event1(
          [|
            {
              "nativeEvent": {
                "contentOffset": {
                  y: scrollYAnimatedValue->React.Ref.current,
                },
              },
            },
          |],
          eventOptions(~useNativeDriver=true, ()),
        )
      )>
      <StickyHeader
        scrollYAnimatedValue={scrollYAnimatedValue->React.Ref.current}
        // scrollOffsetY=0.
        safeArea=false
        backgroundElement={<StickyHeaderBackground />}
        // animateBackgroundOpacity=`yes
        color={themeColors.blue}
        color2={themeColors.blue}
        textStyle=themeStyles##textOnBackground
        title=Filters.title
        right={({color, defaultStyle}) =>
          <TouchableOpacity
            onPress={_ => navigation->Navigators.RootStack.Navigation.goBack()}>
            <Text style=Style.(array([|defaultStyle, style(~color, ())|]))>
              "Done"->React.string
            </Text>
          </TouchableOpacity>
        }
        // rightAlwaysVisible=true
      />
      <Spacer size=XL />
      <Filters />
    </Animated.ScrollView>
  </>;
};
