window.^shorthandTitle^ = (config) => {
  let utils = window["optimizely"].get("utils")

  utils
    .waitUntil(() => {
      return document.body
    })
    .then(() => {
      try {
        let ab = {
          init() {
            let css = "%CSS%",
              style = document.createElement("style")
            style.styleSheet ? (style.styleSheet.cssText = css) : style.appendChild(document.createTextNode(css))
            document.querySelector("head").appendChild(style)

            ab[config.version]()

            document.documentElement.className += ` ${config.slug} ${config.slug}--${config.version} ${config.slug}--loaded ${config.slug}--%BN% `
          },

          a() {
            utils.poll(function () {}, 500)
          },
        }

        if (!document.documentElement.classList.contains(`${config.slug}--loaded`)) {
          ab.init()
        }
      } catch (error) {
        console.log(error)

        document.documentElement.classList.add(`${config.slug}--loaded`)
      }
    })
}
