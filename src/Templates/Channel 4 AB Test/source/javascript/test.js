window.^shorthandTitle^ = (config) => {
  let observer = new MutationObserver(() => {
    try {
      let ab = {
        init() {
          observer.disconnect()

          let css = "%CSS%",
            style = document.createElement("style")
          style.styleSheet ? (style.styleSheet.cssText = css) : style.appendChild(document.createTextNode(css))
          document.querySelector("head").appendChild(style)

          ab[config.version]()

          // Optional - Remove if not required (SPA's sometimes need this)
          this.observe()

          document.documentElement.className += ` ${config.slug} ${config.slug}--${config.version} ${config.slug}--loaded ${config.slug}--%BN% `
        },

        // Optional - Remove if not required (SPA's sometimes need this)
        observe() {
          if (!document.documentElement.classList.contains(`${config.slug}--observing`)) {
            document.documentElement.classList.add(`${config.slug}--observing`)

            setInterval(() => {
              if (!ab.shouldRun() && document.documentElement.classList.contains(`${config.slug}--loaded`)) {
                document.documentElement.classList.remove(`${config.slug}--loaded`)
              } else if (!document.documentElement.classList.contains(`${config.slug}--loaded`) && ab.shouldRun()) {
                ab.init()
              }
            }, 250)
          }
        },

        shouldRun() {
          return document.body
        },

        a() {},
      }

      // Start test if elements available and not already loaded
      if (!document.documentElement.classList.contains(`${config.slug}--loaded`) && ab.shouldRun()) {
        ab.init()
      }
    } catch (error) {
      console.log(error)

      document.documentElement.classList.add(`${config.slug}--loaded`)
    }
  })

  observer.observe(document.querySelector("html"), { attributes: true, childList: true, subtree: true })

  setTimeout(() => {
    document.documentElement.classList.add(`${config.slug}--loaded`)
  }, 10000)
}
