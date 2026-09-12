-- Optional lazy.nvim setup for the bundled Dolphin integration.
-- Replace this path with the location of your Dolphin checkout.
local dolphin_dap_path = vim.fn.expand("/path/to/dolphin/Tools/dap/nvim")

return {
  {
    "mfussenegger/nvim-dap",
    dependencies = {
      "rcarriga/nvim-dap-ui",
      "nvim-neotest/nvim-nio",
    },
    config = function()
      vim.opt.rtp:append(dolphin_dap_path)
      require("dolphin-dap").setup()
    end,
  },
}
