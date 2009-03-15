function wq_execute(data)
  print(data)
  file = io.open("tests/fixtures/lua_ran.txt", "w")
  file:write(os.date("%x"))
  file:close()
  return 1
end
